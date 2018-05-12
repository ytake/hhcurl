<?hh // strict

namespace HHCurl;

use HHCurl\Exceptions\HHCurlException;
use HHCurl\Exceptions\TimeOutException;

type HeaderFunction = (function(resource, string): int);

class Curl {

  const string UserAgent = 'HHVM/Hack HHCurl (+https://github.com/kubotak-is/hhcurl)';

  public function __construct(private ?string $url = null) 
  {}

  public function buildClient(ClientOption $option): Request {
    $ua = $option->getOption()->get(\CURLOPT_USERAGENT);
    if (\is_null($ua)) {
      $option->getOption()->add(Pair{\CURLOPT_USERAGENT, self::UserAgent});
    }
    return new Request($this->url, $option);
  }
}
