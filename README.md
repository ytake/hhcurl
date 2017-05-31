# HHVM/Hack Curl Class

This library provides an object-oriented wrapper of the HHVM/Hack cURL extension.

If you have questions or problems with installation or usage [create an Issue](https://github.com/kubotak-is/hhcurl/issues).

## Installation

In order to install this library via composer run the following command in the console:

```sh
$ hhvm --php $(which composer) require kubotak-is/hhcurl
```

or add the package manually to your composer.json file in the require section:

```json
"kubotak-is/hhcurl": "^0.1"
```

## Usage examples

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->get('http://www.example.com/'));
```

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->get('http://www.example.com/search', [
    'q' => 'keyword',
]));
```

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->post('http://www.example.com/login/', [
    'username' => 'myusername',
    'password' => 'mypassword',
]));
```

```php
$curl = new HHCurl\Curl();
$curl->setBasicAuthentication('username', 'password');
$curl->setUserAgent('');
$curl->setReferrer('');
$curl->setHeader('X-Requested-With', 'XMLHttpRequest');
$curl->setCookie('key', 'value');
\HH\Asio\join($curl->get('http://www.example.com/'));

if ($curl->error) {
    echo $curl->error_code;
}
else {
    echo $curl->response;
}

var_dump($curl->request_headers);
var_dump($curl->response_headers);
```

```php
$curl = new HHCurl\Curl();
$curl->setOpt(CURLOPT_RETURNTRANSFER, TRUE);
$curl->setOpt(CURLOPT_SSL_VERIFYPEER, FALSE);
\HH\Asio\join($curl->get('https://encrypted.example.com/'));
```

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->put('http://api.example.com/user/', [
    'first_name' => 'Zach',
    'last_name' => 'Borboa',
]));
```

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->patch('http://api.example.com/profile/', [
    'image' => '@path/to/file.jpg',
]));
```

```php
$curl = new HHCurl\Curl();
\HH\Asio\join($curl->delete('http://api.example.com/user/', [
    'id' => '1234',
]));
```

```php
$curl->close();
```

```php
// Example access to curl object.
curl_set_opt($curl->curl, CURLOPT_USERAGENT, 'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1');
curl_close($curl->curl);
```

## Testing

In order to test the library:

1. Create a fork
2. Clone the fork to your machine
3. Install the depencies `composer install`
4. Run the unit tests `./vendor/bin/phpunit tests`
