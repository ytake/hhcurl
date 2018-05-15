<?hh // strict

namespace HHCurl;

/**
 * curl request class
 */
class Request {
  
  protected Map<int, mixed> $requestOption = Map{};
  private Map<string, mixed> $responseHeaders = Map{};
  protected array<string, array<mixed>> $responseHeaderArray = [];
  protected ?resource $curl;

  public function __construct(
    private ?string $url, 
    protected ClientOption $option
  ) {}

  protected function preparePayload(AbstractParameter $params): void {
    $this->requestOption->add(Pair{\CURLOPT_POST, true});
    $params = \strval($params);
    $this->requestOption->add(Pair{\CURLOPT_POSTFIELDS, $params});
  }

  protected function setUrl(
    ?string $url,
    ?AbstractParameter $params,
  ): void {
    $this->url = $url;
    $this->requestOption->add(Pair{\CURLOPT_URL, $url});
    if($params instanceof UrlQueryParameter) {
      $params = \strval($params);
      if ($params !== '') {
        $this->requestOption->add(Pair{
          \CURLOPT_URL, $url . '?' . $params
        });
      }
    }
  }

  public async function get(
    string $url,
    ?AbstractParameter $query = null,
  ): Awaitable<Response> {
    $this->setUrl($url, $query);
    $this->requestOption->add(Pair{\CURLOPT_HTTPGET, true});
    $r = $this->exec();
    return await $r;
  }

  public async function post(
    string $url,
    AbstractParameter $params
  ): Awaitable<Response> {
    $this->setUrl($url, null);
    $this->preparePayload($params);
    return await $this->exec();
  }

  public async function put(
    string $url,
    AbstractParameter $params,
    bool $payload = false,
  ): Awaitable<Response> {
    if ($payload === true) {
      $this->preparePayload($params);
      $params = null;
    }
    $this->setUrl($url, $params);
    $this->requestOption->add(Pair{\CURLOPT_CUSTOMREQUEST, 'PUT'});
    return await $this->exec();
  }

  public async function patch(
    string $url,
    AbstractParameter $params,
    bool $payload = false,
  ): Awaitable<Response> {
    if ($payload === true) {
      $this->preparePayload($params);
      $params = null;
    }
    $this->setUrl($url, $params);
    $this->requestOption->add(Pair{\CURLOPT_CUSTOMREQUEST, 'PATCH'});
    return await $this->exec();
  }

  public async function delete(
    string $url,
    AbstractParameter $params,
    bool $payload = false,
  ): Awaitable<Response> {
    if ($payload === true) {
      $this->preparePayload($params);
      $params = null;
    }
    $this->setUrl($url, $params);
    $this->requestOption->add(Pair{\CURLOPT_CUSTOMREQUEST, 'DELETE'});
    return await $this->exec();
  }

  public async function head(
    string $url,
    ?AbstractParameter $params,
  ): Awaitable<Response> {
    $this->setUrl($url, $params);
    $this->requestOption
      ->add(Pair{\CURLOPT_HTTPGET, true})
      ->add(Pair{\CURLOPT_NOBODY, true});
    return await $this->exec();
  }

  protected async function exec(): Awaitable<Response> {
    $this->curl = \curl_init();

    if(!$this->option->getOption()->get(\CURLOPT_HEADERFUNCTION)) {
      $this->setHeaderFunction($this->defaultHeaderFunction());
    }
    foreach($this->option->getOption()->toArray() as $option => $value) {
      \curl_setopt($this->curl, $option, $value);
    }
    foreach($this->requestOption->toArray() as $option => $value) {
      \curl_setopt($this->curl, $option, $value);
    }
    $cookie = $this->option->getCookies();
    if ($cookie->count()) {
      \curl_setopt(
        $this->curl, 
        \CURLOPT_COOKIE, 
        \http_build_query($cookie->toArray(), '', '; ')
      );
    }
    if(\is_resource($this->curl)) {
      $handler = new CurlHandler($this->curl, $this->responseHeaders);
      return await $handler->handle();
    }
    throw new \RuntimeException();
  }
  
  public function curl(): ?resource {
    return $this->curl;
  }

  public function setHeaderFunction(HeaderFunction $callback): void {
    $this->option->getOption()->add(Pair{\CURLOPT_HEADERFUNCTION, $callback});
  }
  
  protected function defaultHeaderFunction(): HeaderFunction {
    return (resource $curl, string $headerLine) ==> {
      $trimmedHeader = \trim($headerLine, "\r\n");
      if ($trimmedHeader === "") {
        $this->option->setResponseHeaderContinue(false);
      } else if (\strtolower($trimmedHeader) === 'http/1.1 100 continue') {
        $this->option->setResponseHeaderContinue(true);
      } else if (!$this->option->getResponseHeaderContinue()) {
        $header = \explode(':', $trimmedHeader, 2);
        if (\count($header) < 3) {
          if(\count($header) == 2) {
            $name = \strtolower(\trim($header[0]));
            if (!\array_key_exists($name, $this->responseHeaderArray)) {
              $this->responseHeaderArray[$name] = [\trim($header[1])];
            } else {
              $this->responseHeaderArray[$name][] = \trim($header[1]);
            }
          }
        }
      }
      $this->responseHeaders->setAll($this->responseHeaderArray);
      return \strlen($headerLine);
    };
  }
  
  public function getResponseHeader(): Map<string, mixed> {
    return $this->responseHeaders;  
  }
}
