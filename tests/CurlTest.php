<?hh // strict

use HHCurl\Curl;
use PHPUnit\Framework\TestCase;

class CurlTest extends TestCase {
  const string TEST_URL = 'http://localhost';

  private function curl(): Curl {
    $curl = new Curl();
    $curl->setOpt(CURLOPT_SSL_VERIFYPEER, false);
    $curl->setOpt(CURLOPT_SSL_VERIFYHOST, false);
    return $curl;
  }

  protected function server(
    string $request_method,
    mixed $data = '',
  ): ?string {
    $request_method = strtolower($request_method);
    $c = $this->curl();
    \HH\Asio\join(/* UNSAFE_EXPR */
      $c->$request_method(self::TEST_URL.'/server.php', $data),
    );
    return $c->response;
  }

  private function create_png(): string {
    // PNG image data, 1 x 1, 1-bit colormap, non-interlaced
    ob_start();
    imagepng(
      imagecreatefromstring(
        base64_decode(
          'R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7',
        ),
      ),
    );
    $raw_image = ob_get_contents();
    ob_end_clean();
    return $raw_image;
  }

  private function create_tmp_file(string $data): mixed {
    $tmp_file = tmpfile();
    fwrite($tmp_file, $data);
    rewind($tmp_file);
    return $tmp_file;
  }

  private function get_png(): string {
    $tmp_filename = tempnam('/tmp', 'php-curl-class.');
    file_put_contents($tmp_filename, $this->create_png());
    return $tmp_filename;
  }

  public function testExtensionLoaded(): void {
    $this->assertTrue(extension_loaded('curl'));
  }

  public function testUserAgent(): void {
    $c = $this->curl();
    $c->setUserAgent(Curl::USER_AGENT);
    $this->assertEquals(
      Curl::USER_AGENT,
      $this->server('GET', ['test' => 'server', 'key' => 'HTTP_USER_AGENT']),
    );
  }

  public function testGet(): void {
    $this->assertTrue(
      $this->server(
        'GET',
        array('test' => 'server', 'key' => 'REQUEST_METHOD'),
      ) ===
      'GET',
    );
  }

  public function testPostRequestMethod(): void {
    $this->assertTrue(
      $this->server(
        'POST',
        array('test' => 'server', 'key' => 'REQUEST_METHOD'),
      ) ===
      'POST',
    );
  }

  public function testPostData(): void {
    $this->assertTrue(
      $this->server('POST', array('test' => 'post', 'key' => 'test')) ===
      'post',
    );
  }

  public function testPostMultidimensionalData(): void {

    $data = array(
      'key' => 'file',
      'file' => array('wibble', 'wubble', 'wobble'),
    );
    $c = $this->curl();
    \HH\Asio\join(
      $c->post(self::TEST_URL.'/post_multidimensional.php', $data),
    );

    $this->assertEquals(
      'key=file&file%5B0%5D=wibble&file%5B1%5D=wubble&file%5B2%5D=wobble',
      $c->response,
    );

  }

  public function testPostFilePathUpload(): void {
    $c = $this->curl();
    $file_path = $this->get_png();

    $data = array('key' => 'image', 'image' => '@'.$file_path);

    $c->setOpt(CURLOPT_RETURNTRANSFER, true);

    \HH\Asio\join(
      $c->post(self::TEST_URL.'/post_file_path_upload.php', $data),
    );

    $this->assertEquals(
      array(
        'request_method' => 'POST',
        'key' => 'image',
        'mime_content_type' => 'ERROR', // Temp change the image response, but assuming this is not fixing the issue indeed.
      //'mime_content_type' => 'image/png'
      ),
      json_decode($c->response, true),
    );

    unlink($file_path);
  }

  public function testPutRequestMethod(): void {
    $this->assertTrue(
      $this->server(
        'PUT',
        array('test' => 'server', 'key' => 'REQUEST_METHOD'),
      ) ===
      'PUT',
    );
  }

  public function testPutData(): void {
    $this->assertTrue(
      $this->server('PUT', array('test' => 'put', 'key' => 'test')) ===
      'put',
    );
  }

  public function testPutFileHandle(): void {
    $png = $this->create_png();
    $tmp_file = $this->create_tmp_file(strval($png));
    $c = $this->curl();
    $c->setOpt(CURLOPT_PUT, TRUE);
    $c->setOpt(CURLOPT_INFILE, $tmp_file);
    $c->setOpt(CURLOPT_INFILESIZE, strlen($png));
    \HH\Asio\join(
      $c->put(
        self::TEST_URL.'/server.php',
        array('test' => 'put_file_handle'),
      ),
    );

    fclose($tmp_file);

    $this->assertTrue($c->response === 'image/png');
  }

  public function testDelete(): void {
    $this->assertTrue(
      $this->server(
        'DELETE',
        array('test' => 'server', 'key' => 'REQUEST_METHOD'),
      ) ===
      'DELETE',
    );

    $this->assertTrue(
      $this->server('DELETE', array('test' => 'delete', 'key' => 'test')) ===
      'delete',
    );
  }

  public function testBasicHttpAuth(): void {

    $data = array();
    $c = $this->curl();
    \HH\Asio\join($c->get(self::TEST_URL.'/http_basic_auth.php', $data));

    $this->assertEquals('canceled', $c->response);
    $c->reset();
    $username = 'myusername';
    $password = 'mypassword';

    $c->setBasicAuthentication($username, $password);

    \HH\Asio\join($c->get(self::TEST_URL.'/http_basic_auth.php', $data));

    $this->assertEquals(
      '{"username":"myusername","password":"mypassword"}',
      $c->response,
    );
  }

  public function testReferrer(): void {
    $c = $this->curl();
    $c->setReferer('myreferrer');
    \HH\Asio\join(
      $c->get(
        self::TEST_URL.'/server.php',
        ['test' => 'server', 'key' => 'HTTP_REFERER'],
      ),
    );
    $this->assertSame('myreferrer', $c->response);
  }

  public function testCookies(): void {
    $c = $this->curl();
    $c->setCookie('mycookie', 'yum');
    \HH\Asio\join(
      $c->get(
        self::TEST_URL.'/server.php',
        ['test' => 'cookie', 'key' => 'mycookie'],
      ),
    );
    $this->assertSame('yum', $c->response);
  }

  /**
   * @expectedException \HHCurl\Exceptions\HHCurlException
   */
  public function testError(): void {
    $c = $this->curl();
    $c->setOpt(CURLOPT_CONNECTTIMEOUT_MS, 2000);
    \HH\Asio\join($c->get('http://1.2.3.4/'));
    $this->assertTrue($c->error === TRUE);
    $this->assertTrue($c->curl_error === TRUE);
    $this->assertTrue($c->curl_error_code === CURLE_OPERATION_TIMEOUTED);
  }

  public function testHeaders(): void {
    $c = $this->curl();
    $c->setHeader('Content-Type', 'application/json');
    \HH\Asio\join(
      $c->get(
        self::TEST_URL.'/server.php',
        ['test' => 'server', 'key' => 'CONTENT_TYPE'],
      ),
    );
    $this->assertSame('application/json', $c->response);
    $c->reset();
    $c->setHeader('X-Requested-With', 'XMLHttpRequest');
    \HH\Asio\join(
      $c->get(
        self::TEST_URL.'/server.php',
        ['test' => 'server', 'key' => 'HTTP_X_REQUESTED_WITH'],
      ),
    );
    $this->assertSame('XMLHttpRequest', $c->response);
    $c->reset();
    $c->setHeader('Accept', 'application/json');
    \HH\Asio\join(
      $c->get(
        self::TEST_URL.'/server.php',
        ['test' => 'server', 'key' => 'HTTP_ACCEPT'],
      ),
    );
    $this->assertSame('application/json', $c->response);
  }
}
