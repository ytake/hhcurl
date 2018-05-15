<?hh // strict

use HHCurl\Curl;
use HHCurl\Request;
use HHCurl\Response;
use HHCurl\ClientOption;
use PHPUnit\Framework\TestCase;

class CurlTest extends TestCase {
  const string TEST_URL = 'http://127.0.0.1:8080/server.php';

  public function testShouldReturunRequestInstance(): void {
    $curl = new Curl();
    $this->assertInstanceOf(
      Request::class,
      $curl->buildClient(new ClientOption())
    );
  }

  public function testShouldCall(): void {
    $curl = new Curl();
    $client = $curl->buildClient(new ClientOption());
    $response = \HH\Asio\join($client->get(self::TEST_URL));
    $this->assertSame(self::TEST_URL, $response->effectiveUrl());
    $this->assertSame(200, $response->getHttpStatusCode());
    $this->assertSame(0, $response->getErrorCode());
    $this->assertSame('testing', $response->getResponse());
    $this->assertSame(['testing'], $response->getResponseHeader()->get('x-app-message'));
  }
}
