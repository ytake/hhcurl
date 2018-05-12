<?hh // strict

namespace HHCurl;

/**
 * for example
 *
 * using new \HHCurl\Close($curlHandler);
 */
final class HandleClose implements \IDisposable {
  
  public function __construct(
    private resource $curl,
  ) {}

  public function __dispose(): void {
    \curl_close($this->curl);
  }
}
