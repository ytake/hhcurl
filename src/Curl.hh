<?hh // strict

namespace HHCurl;

use HHCurl\Exceptions\HHCurlException;
use HHCurl\Exceptions\TimeOutException;

class Curl {
  const int AUTH_BASIC = CURLAUTH_BASIC;
  const int AUTH_DIGEST = CURLAUTH_DIGEST;
  const int AUTH_GSSNEGOTIATE = CURLAUTH_GSSNEGOTIATE;
  const int AUTH_NTLM = CURLAUTH_NTLM;
  const int AUTH_ANY = CURLAUTH_ANY;
  const int AUTH_ANYSAFE = CURLAUTH_ANYSAFE;
  const string
    USER_AGENT = 'HHVM/Hack HHCurl (+https://github.com/kubotak-is/hhcurl)';

  private array<mixed, mixed> $_cookies = [];
  private array<mixed, mixed> $_headers = [];

  public resource $curl;

  public bool $error = false;

  public int $error_code = 0;

  public ?string $error_message = null;

  public bool $curl_error = false;

  public int $curl_error_code = 0;

  public ?string $curl_error_message = null;

  public bool $http_error = false;

  public int $http_status_code = 0;

  public ?string $http_error_message = null;

  public mixed $request_headers = null;

  public Map<mixed, mixed> $response_headers = Map {};

  public ?string $response = null;

  private int $timeout = 60;

  private string $url = '';

  protected bool $response_header_continue = false;

  private int $retry_count = 2;

  /**
   * Constructor ensures the available curl extension is loaded.
   *
   * @throws \ErrorException
   */
  public function __construct() {
    $this->init();
  }

  // @deprecated
  public function getInstance(): Curl {
    return new self();
  }

  // private methods

  /**
   * Initializer for the curl resource.
   *
   * Is called by the __construct() of the class or when the curl request is reseted.
   * @return self
   */
  private function init(): this {
    $this->curl = curl_init();
    $this->setUserAgent(self::USER_AGENT);
    $this->setOpt(CURLINFO_HEADER_OUT, true);
    $this->setOpt(CURLOPT_HEADER, false);
    $this->setOpt(CURLOPT_RETURNTRANSFER, true);
    $this->setOpt(CURLOPT_HEADERFUNCTION, [$this, 'addResponseHeaderLine']);
    return $this;
  }

  /**
   * Execute the curl request based on the respectiv settings.
   *
   * @return int Returns the error code for the current curl request
   */
  protected async function exec(): Awaitable<int> {
    $this->response_headers = Map {};

    $mh = curl_multi_init();
    curl_multi_add_handle($mh, $this->curl);
    $sleep_ms = 10;
    $try_count = 0;
    do {
      $active = 1;
      do {
        $status = curl_multi_exec($mh, $active);
      } while ($status == CURLM_CALL_MULTI_PERFORM);
      if (!$active) break;
      $select = await curl_multi_await($mh, (float) $this->timeout);
      /* If cURL is built without ares suppor, DNS queries don't have a socket
      * to wait on, so curl_multi_await() (and curl_select() in PHP5) will return
      * -1, and polling is required.
      */
      // BUGFIX secret is iterate -1 are Abort HHVM
      if ($try_count >= $this->retry_count && $select === -1) {
        throw new TimeOutException("TimeOut Curl: {$this->timeout} sec, [url] {$this->url}");
      }
      if ($select === -1) {
        $try_count++;
        await SleepWaitHandle::create($sleep_ms * 1000);
        if ($sleep_ms < 1000) {
          $sleep_ms *= 2;
        }
      } else {
        $sleep_ms = 10;
      }
    } while ($status === CURLM_OK);

    $this->response = (string) curl_multi_getcontent($this->curl);
    $this->curl_error_code = curl_errno($this->curl);
    $this->curl_error_message = curl_error($this->curl);
    $this->curl_error = !($this->curl_error_code === 0);
    $this->http_status_code = curl_getinfo($this->curl, CURLINFO_HTTP_CODE);
    $this->http_error =
      in_array(floor($this->http_status_code / 100), [4, 5]);
    $this->error = $this->curl_error || $this->http_error;
    $this->error_code =
      $this->error
        ? ($this->curl_error
             ? $this->curl_error_code
             : $this->http_status_code)
        : 0;
    $ci = curl_getinfo($this->curl, CURLINFO_HEADER_OUT);

    curl_multi_remove_handle($mh, $this->curl);
    curl_multi_close($mh);
    curl_close($this->curl);

    if ($ci === false) {
      throw new HHCurlException("Failed Curl Get Info, [url] {$this->url}");
    }

    $this->request_headers =
      preg_split('/\r\n/', $ci, null, PREG_SPLIT_NO_EMPTY);

    $httpErrorMessage = $this->response_headers->get('0');
    if (is_null($httpErrorMessage)) {
      $httpErrorMessage = '';
    }
    $this->http_error_message = $this->error ? strval($httpErrorMessage) : '';
    $this->error_message =
      $this->curl_error
        ? $this->curl_error_message
        : $this->http_error_message;

    if (is_null($this->response) || $this->response === '') {
      if (!$this->curl_error && $this->http_status_code === 0) {
        throw new TimeOutException(
          "TimeOut Curl: {$this->timeout} sec, [url] {$this->url}",
        );
      }
    }
    return $this->error_code;
  }

  /**
   * Handle writing the response headers
   *
   * @param resource $curl The current curl resource
   * @param string $header_line A line from the list of response headers
   *
   * @return int Returns the length of the $header_line
   */
  public function addResponseHeaderLine(
    resource $curl,
    string $headerLine,
  ): int {
    $trimmedHeader = trim($headerLine, "\r\n");
    if ($trimmedHeader === "") {
      $this->response_header_continue = false;
    } else if (strtolower($trimmedHeader) === 'http/1.1 100 continue') {
      $this->response_header_continue = true;
    } else if (!$this->response_header_continue) {
      $this->response_headers->set($trimmedHeader, $trimmedHeader);
    }

    return strlen($headerLine);
  }

  /**
   * @param array|object|string $data
   */
  protected function preparePayload(mixed $data): void {
    $this->setOpt(CURLOPT_POST, true);
    if (is_null($data)) {
      $data = '';
    }
    if (is_array($data) || is_object($data)) {
      $data = http_build_query($data);
    }
    $value = strval($data);
    $this->setOpt(CURLOPT_POSTFIELDS, $value);
  }

  /**
   * Set auth options for the current request.
   *
   * Available auth types are:
   *
   * + self::AUTH_BASIC
   * + self::AUTH_DIGEST
   * + self::AUTH_GSSNEGOTIATE
   * + self::AUTH_NTLM
   * + self::AUTH_ANY
   * + self::AUTH_ANYSAFE
   *
   * @param int $httpauth The type of authentication
   */
  protected function setHttpAuth(int $httpauth): void {
    $this->setOpt(CURLOPT_HTTPAUTH, $httpauth);
  }

  /**
   * @param string $url
   * @param array $data = []
   */
  protected function setUrl(
    string $url,
    array<mixed, mixed> $data = [],
  ): void {
    $this->url = $url;
    $this->setOpt(CURLOPT_URL, $url);
    if (count($data) > 0) {
      $this->setOpt(CURLOPT_URL, $url.'?'.http_build_query($data));
    }
  }

  /**
   * Make a get request with optional data.
   *
   * The get request has no body data, the data will be correctly added to the $url with the http_build_query() method.
   *
   * @param string $url  The url to make the get request for
   * @param array  $data Optional arguments who are part of the url
   * @return self
   */
  public async function get(
    string $url,
    array<mixed, mixed> $data = [],
  ): Awaitable<int> {
    $this->setUrl($url, $data);
    $this->setOpt(CURLOPT_HTTPGET, true);
    return await $this->exec();
  }

  /**
   * Make a post request with optional post data.
   *
   * @param string $url  The url to make the get request
   * @param mixed  $data Post data to pass to the url
   * @return self
   */
  public async function post(string $url, mixed $data = []): Awaitable<int> {
    $this->setUrl($url);
    $this->preparePayload($data);
    return await $this->exec();
  }

  /**
   * Make a put request with optional data.
   *
   * The put request data can be either sent via payload or as get paramters of the string.
   *
   * @param string $url     The url to make the get request
   * @param mixed  $data    Optional data to pass to the $url
   * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
   * @return self
   */
  public async function put(
    string $url,
    mixed $data = [],
    bool $payload = false,
  ): Awaitable<int> {
    if (!is_null($data) || $data !== '') {
      if ($payload === false) {
        $url .= '?'.http_build_query($data);
      } else {
        $this->preparePayload($data);
      }
    }

    $this->setUrl($url);
    $this->setOpt(CURLOPT_CUSTOMREQUEST, 'PUT');
    return await $this->exec();
  }

  /**
   * Make a patch request with optional data.
   *
   * The patch request data can be either sent via payload or as get paramters of the string.
   *
   * @param string $url     The url to make the get request
   * @param mixed  $data    Optional data to pass to the $url
   * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
   * @return self
   */
  public async function patch(
    string $url,
    mixed $data = [],
    bool $payload = false,
  ): Awaitable<int> {
    if (!is_null($data) || $data !== '') {
      if ($payload === false) {
        $url .= '?'.http_build_query($data);
      } else {
        $this->preparePayload($data);
      }
    }

    $this->setUrl($url);
    $this->setOpt(CURLOPT_CUSTOMREQUEST, 'PATCH');
    return await $this->exec();
  }

  /**
   * Make a delete request with optional data.
   *
   * @param string $url     The url to make the delete request
   * @param mixed  $data    Optional data to pass to the $url
   * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
   * @return self
   */
  public async function delete(
    string $url,
    mixed $data = [],
    bool $payload = false,
  ): Awaitable<int> {
    if (!is_null($data) || $data !== '') {
      if ($payload === false) {
        $url .= '?'.http_build_query($data);
      } else {
        $this->preparePayload($data);
      }
    }

    $this->setUrl($url);
    $this->setOpt(CURLOPT_CUSTOMREQUEST, 'DELETE');
    return await $this->exec();
  }

  /**
   * Make a head request with optional data.
   *
   * The get request has no body data, the data will be correctly added to the $url with the http_build_query() method.
   *
   * @param string $url  The url to make the get request for
   * @param array  $data Optional arguments who are part of the url
   * @return self
   */
  public async function head(
    string $url,
    array<mixed, mixed> $data = [],
  ): Awaitable<int> {
    $this->setUrl($url, $data);
    $this->setOpt(CURLOPT_HTTPGET, true);
    $this->setOpt(CURLOPT_NOBODY, true);
    return await $this->exec();
  }

  // setters

  /**
   * Pass basic auth data.
   *
   * If the the rquested url is secured by an httaccess basic auth mechanism you can use this method to provided the auth data.
   *
   * ```php
   * $curl = new Curl();
   * $curl->setBasicAuthentication('john', 'doe');
   * $curl->get('http://example.com/secure.php');
   * ```
   *
   * @param string $username The username for the authentification
   * @param string $password The password for the given username for the authentification
   * @return self
   */
  public function setBasicAuthentication(
    string $username,
    string $password,
  ): this {
    $this->setHttpAuth(self::AUTH_BASIC);
    $this->setOpt(CURLOPT_USERPWD, $username.':'.$password);
    return $this;
  }

  /**
   * Provide optional header informations.
   *
   * In order to pass optional headers by key value pairing:
   *
   * ```php
   * $curl = new Curl();
   * $curl->setHeader('X-Requested-With', 'XMLHttpRequest');
   * $curl->get('http://example.com/request.php');
   * ```
   *
   * @param string $key   The header key
   * @param string $value The value for the given header key
   * @return self
   */
  public function setHeader(string $key, string $value): this {
    $this->_headers[$key] = $key.': '.$value;
    $this->setOpt(CURLOPT_HTTPHEADER, array_values($this->_headers));
    return $this;
  }

  /**
   * Provide a User Agent.
   *
   * In order to provide you cusomtized user agent name you can use this method.
   *
   * ```php
   * $curl = new Curl();
   * $curl->setUserAgent('My John Doe Agent 1.0');
   * $curl->get('http://example.com/request.php');
   * ```
   *
   * @param string $useragent The name of the user agent to set for the current request
   * @return self
   */
  public function setUserAgent(string $useragent): this {
    $this->setOpt(CURLOPT_USERAGENT, $useragent);
    return $this;
  }

  /**
   * Set the HTTP referer header.
   *
   * The $referer informations can help identify the requested client where the requested was made.
   *
   * @param string $referer An url to pass and will be set as referer header
   * @return self
   */
  public function setReferer(string $referer): this {
    $this->setOpt(CURLOPT_REFERER, $referer);
    return $this;
  }

  /**
   * Set cUrl Time Out
   *
   * @param ?int $secound
   * @return self
   */
  public function setTimeOut(int $secound = 5): this {
    $this->timeout = $secound;
    $this->setOpt(CURLOPT_TIMEOUT, $this->timeout);
    return $this;
  }

  /**
   * Set Curl Retry Count
   *
   * @param ?int $count
   * @return self
   */
  public function setRetryCount(int $count = 2): this {
      $this->retry_count = $count;
      return $this;
  }

  /**
   * Set contents of HTTP Cookie header.
   *
   * @param string $key   The name of the cookie
   * @param string $value The value for the provided cookie name
   * @return self
   */
  public function setCookie(string $key, string $value): this {
    $this->_cookies[$key] = $value;
    $this->setOpt(
      CURLOPT_COOKIE,
      http_build_query($this->_cookies, '', '; '),
    );
    return $this;
  }

  /**
   * Set customized curl options.
   *
   * To see a full list of options: http://php.net/curl_setopt
   *
   * @see http://php.net/curl_setopt
   *
   * @param int   $option The curl option constante e.g. `CURLOPT_AUTOREFERER`, `CURLOPT_COOKIESESSION`
   * @param mixed $value  The value to pass for the given $option
   */
  public function setOpt(int $option, mixed $value): bool {
    return curl_setopt($this->curl, $option, $value);
  }

  /**
   * Get customized curl options.
   *
   * To see a full list of options: http://php.net/curl_getinfo
   *
   * @see http://php.net/curl_getinfo
   *
   * @param int   $option The curl option constante e.g. `CURLOPT_AUTOREFERER`, `CURLOPT_COOKIESESSION`
   * @param mixed $value  The value to check for the given $option
   */
  public function getOpt(int $option): mixed {
    return curl_getinfo($this->curl, $option);
  }

  /**
   * Return the endpoint set for curl
   *
   * @see http://php.net/curl_getinfo
   *
   * @return string of endpoint
   */
  public function getEndpoint(): mixed {
    return $this->getOpt(CURLINFO_EFFECTIVE_URL);
  }

  /**
   * Enable verbositiy.
   *
   * @todo As to keep naming convention it should be renamed to `setVerbose()`
   *
   * @param string $on
   * @return self
   */
  public function verbose(bool $on = true): this {
    $this->setOpt(CURLOPT_VERBOSE, $on);
    return $this;
  }

  /**
   * Reset all curl options.
   *
   * In order to make multiple requests with the same curl object all settings requires to be reset.
   * @return self
   */
  public function reset(): this {
    $this->close();
    $this->_cookies = [];
    $this->_headers = [];
    $this->error = false;
    $this->error_code = 0;
    $this->error_message = null;
    $this->curl_error = false;
    $this->curl_error_code = 0;
    $this->curl_error_message = null;
    $this->http_error = false;
    $this->http_status_code = 0;
    $this->http_error_message = null;
    $this->request_headers = null;
    $this->response_headers = Map {};
    $this->response = null;
    $this->setTimeOut(60);
    $this->url = '';
    $this->init();
    return $this;
  }

  /**
   * Closing the current open curl resource.
   * @return self
   */
  public function close(): this {
    if (is_resource($this->curl)) {
      curl_close($this->curl);
    }
    return $this;
  }

  /**
   * Close the connection when the Curl object will be destroyed.
   */
  public function __destruct() {
    $this->close();
  }

  /**
   * Was an 'info' header returned.
   * @return bool
   */
  public function isInfo(): bool {
    return $this->http_status_code >= 100 && $this->http_status_code < 200;
  }

  /**
   * Was an 'OK' response returned.
   * @return bool
   */
  public function isSuccess(): bool {
    return $this->http_status_code >= 200 && $this->http_status_code < 300;
  }

  /**
   * Was a 'redirect' returned.
   * @return bool
   */
  public function isRedirect(): bool {
    return $this->http_status_code >= 300 && $this->http_status_code < 400;
  }

  /**
   * Was an 'error' returned (client error or server error).
   * @return bool
   */
  public function isError(): bool {
    return $this->http_status_code >= 400 && $this->http_status_code < 600;
  }

  /**
   * Was a 'client error' returned.
   * @return bool
   */
  public function isClientError(): bool {
    return $this->http_status_code >= 400 && $this->http_status_code < 500;
  }

  /**
   * Was a 'server error' returned.
   * @return bool
   */
  public function isServerError(): bool {
    return $this->http_status_code >= 500 && $this->http_status_code < 600;
  }
}
