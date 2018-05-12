<?hh // strict

namespace HHCurl;

use HHCurl\Exceptions\HHCurlException;
use HHCurl\Exceptions\TimeOutException;

final class CurlHandler {

  public function __construct(
    private resource $curl,
    private Map<mixed, mixed> $responseHeaders
  ) {}
  
  /** 
   * override
   * @see https://github.com/facebook/hhvm/blob/master/hphp/runtime/ext/curl/ext_curl.php#L517
   */
  public async function handle(): Awaitable<Response> {
    $mh = \curl_multi_init();
    \curl_multi_add_handle($mh, $this->curl);
    $sleepMs = 10;
    do {
      $active = 1;
      do {
        $status = \curl_multi_exec($mh, &$active);
      } while ($status == \CURLM_CALL_MULTI_PERFORM);
      if (!$active) break;
      $select = await \curl_multi_await($mh);
      if ($select === -1) {
        await \SleepWaitHandle::create($sleepMs * 1000);
        if ($sleepMs < 1000) {
          $sleepMs *= 2;
        }
      } else {
        $sleepMs = 10;
      }
    } while ($status === \CURLM_OK);
    $effectiveUrl = \curl_getinfo($this->curl, \CURLINFO_EFFECTIVE_URL);
    $response = (string) \curl_multi_getcontent($this->curl);
    $curlErrorCode = \curl_errno($this->curl);
    $curlErrorMessage = \curl_error($this->curl);
    $curlError = !($curlErrorCode === 0);
    $httpStatusCode = \curl_getinfo($this->curl, \CURLINFO_HTTP_CODE);
    $httpError = \in_array(\floor($httpStatusCode / 100), [4, 5]);
    $error = $curlError || $httpError;
    $errorCode = $error ? ($curlError ? $curlErrorCode : $httpStatusCode): 0;
    $ci = \curl_getinfo($this->curl, \CURLINFO_HEADER_OUT);
    \curl_multi_remove_handle($mh, $this->curl);
    \curl_multi_close($mh);
    \curl_close($this->curl);
    if ($ci === false) {
      throw new HHCurlException(
        \sprintf("Failed Curl Get Info, [url] %s", $effectiveUrl)
      );
    }
    $requestHeader = new ImmVector(\preg_split('/\r\n/', $ci, null, \PREG_SPLIT_NO_EMPTY));
    $httpErrorMessage = $this->responseHeaders->get('0');
    if (\is_null($httpErrorMessage)) {
      $httpErrorMessage = '';
    }
    $httpErrorMessage = $error ? \strval($httpErrorMessage) : '';
    $errorMessage = $curlError ? $curlErrorMessage : $httpErrorMessage;
    if (\is_null($response) || $response === '') {
      if (!$curlError && $httpStatusCode === 0) {
        throw new TimeOutException(
          \sprintf("Request TimeOut [url] %s", $effectiveUrl)
        );
      }
    }
    return new Response(
      $effectiveUrl, 
      $errorCode, 
      $response,
      $error,
      $errorMessage,
      $curlError,
      $curlErrorCode,
      $curlErrorMessage,
      $httpError,
      $httpStatusCode,
      $httpErrorMessage,
      $requestHeader,
      $this->responseHeaders->toImmMap()
    );
  }
}
