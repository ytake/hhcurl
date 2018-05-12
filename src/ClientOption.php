<?hh // strict

namespace HHCurl;

enum Auth: int {
  Basic =  \CURLAUTH_BASIC;
  Digest = \CURLAUTH_DIGEST;
  GssNegotiate = \CURLAUTH_GSSNEGOTIATE;
  Ntlm = \CURLAUTH_NTLM;
  Aby = \CURLAUTH_ANY;
  Anysafe = \CURLAUTH_ANYSAFE;
}

class ClientOption {

  protected Map<int, mixed> $clientOption = Map{
    \CURLINFO_HEADER_OUT => true,
    \CURLOPT_HEADER => false,
    \CURLOPT_RETURNTRANSFER => true,
  };

  protected Map<string, string> $headers = Map{};
  
  protected Map<string, string> $cookies = Map{};

  protected bool $responseHeaderContinue = false;

  public function verbose(bool $enable = true): this {
    $this->clientOption->add(Pair{\CURLOPT_VERBOSE, $enable});
    return $this;
  }

  public function setResponseHeaderContinue(bool $isContinue): void {
    $this->responseHeaderContinue = $isContinue;
  }

  public function getResponseHeaderContinue(): bool {
    return $this->responseHeaderContinue;
  }

  public function setHttpAuth(Auth $auth): ClientOption {
    $this->clientOption->add(Pair{\CURLOPT_HTTPAUTH, $auth});
    return $this;
  }

  public function setTimeOut(int $second = 5): ClientOption {
    $this->clientOption->add(Pair{\CURLOPT_TIMEOUT, $second});
    return $this;
  }

  public function setBasicAuthentication(
    string $username,
    string $password,
  ): this {
    $credential = \sprintf("%s:%s", $username, $password);
    $this->clientOption->add(Pair{\CURLOPT_USERPWD, $credential});
    return $this;
  }

  public function getOption(): Map<int, mixed> {
    return $this->clientOption;
  }

  public function getCookies(): Map<string, string> {
    return $this->cookies;
  }

  public function setHeaders(Vector<Pair<string, string>> $headers): this {
    $this->headers->addAll($headers);
    return $this;
  }

  public function setHeader(string $key, string $value): this {
    $this->headers->add(Pair{$key, $key.': '.$value});
    if(!$this->clientOption->at(\CURLOPT_HTTPHEADER)) {
      $this->clientOption->add(Pair{
        \CURLOPT_HTTPHEADER,
        $this->headers->values()->values()
      });
      return $this;
    }
    $v = $this->clientOption->get(\CURLOPT_HTTPHEADER);
    if (\is_array($v)) {
      $this->clientOption->add(Pair{
        \CURLOPT_HTTPHEADER,
        \array_merge($v, $this->headers->values()->values())
      });
    }
    return $this;
  }

  public function setUserAgent(string $ua): this {
    $this->clientOption->add(Pair{\CURLOPT_USERAGENT, $ua});
    return $this;
  }

  public function setReferer(string $referer): this {
    $this->clientOption->add(Pair{\CURLOPT_REFERER, $referer});
    return $this;
  }

  public function setCookie(string $key, string $value): this {
    $this->cookies->add(Pair{$key, $value});
    return $this;
  }
}
