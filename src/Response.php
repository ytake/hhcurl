<?hh // strict

namespace HHCurl;

class Response {

  public function __construct(
    protected string $effectiveUrl, 
    protected int $errorCode = 0, 
    protected ?string $response = null,
    protected bool $error = false,
    protected ?string $errorMessage = null,
    protected bool $curlError = false,
    protected int $curlErrorCode = 0,
    protected ?string $curlErrorMessage = null,
    protected bool $httpError = false,
    protected int $httpStatusCode = 0,
    protected ?string $httpErrorMessage = null,
    protected ImmVector<string> $requestHeader = ImmVector{},
    protected ImmMap<string, mixed> $responseHeaders = ImmMap{}
  ) {}

  public function effectiveUrl(): string {
    return $this->effectiveUrl;
  }
  
  public function getErrorCode(): int {
    return $this->errorCode;
  }

  public function getResponse(): ?string {
    return $this->response;
  }

  public function getError(): bool {
    return $this->error;
  }

  public function getErrorMessage(): ?string {
    return $this->errorMessage;
  }

  public function getCurlError(): bool {
    return $this->curlError;
  }

  public function getCurlErrorCode(): int {
    return $this->curlErrorCode;
  }

  public function getCurlErrorMessage(): ?string {
    return $this->curlErrorMessage;
  }

  public function getHttpError(): bool {
    return $this->httpError;
  }  

  public function getHttpStatusCode(): int {
    return $this->httpStatusCode;
  }  

  public function getHttpErrorMessage(): ?string {
    return $this->httpErrorMessage;
  }

  public function getRequestHeader(): ImmVector<string> {
    return $this->requestHeader;
  }

  public function getResponseHeader(): ImmMap<string, mixed> {
    return $this->responseHeaders;
  } 
}
