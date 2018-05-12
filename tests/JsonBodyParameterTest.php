<?hh // strict

use HHCurl\JsonBodyParameter;
use PHPUnit\Framework\TestCase;

class JsonBodyParameterTest extends TestCase {

  public function testShouldReturunRequestInstance(): void {
    $params = new JsonBodyParameter(new Map([
      'message' => 'test',
      'number' => 1
    ]));
    $this->assertJson(\strval($params));
    $this->assertEquals('{"message":"test","number":1}', \strval($params));
  }
}
