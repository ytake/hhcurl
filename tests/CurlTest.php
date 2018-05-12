<?hh // strict

use HHCurl\Curl;
use HHCurl\Request;
use HHCurl\Response;
use HHCurl\ClientOption;
use PHPUnit\Framework\TestCase;

class CurlTest extends TestCase {
  const string TEST_URL = 'http://localhost';

  public function testShouldReturunRequestInstance(): void {
    $curl = new Curl();
    $this->assertInstanceOf(
      Request::class,
      $curl->buildClient(new ClientOption())
    );
  }
}
