<?hh // strict

use HHCurl\ClientOption;
use PHPUnit\Framework\TestCase;

class ClientOptionTest extends TestCase {

  public function testShouldReturunRequestInstance(): void {
    $option = new ClientOption();
    $o = $option->getOption();
    $this->assertInstanceOf(Map::class, $o);
    $ex = Map{
      \CURLINFO_HEADER_OUT => true,
      \CURLOPT_HEADER => false,
      \CURLOPT_RETURNTRANSFER => true,
    };
    $this->assertSame($ex->toArray(), $o->toArray());
  }
}
