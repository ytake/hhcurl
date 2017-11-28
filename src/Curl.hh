<?hh

namespace HHCurl;

use HHCurl\Exceptions\HHCurlException;
use HHCurl\Exceptions\TimeOutException;

/**
 * An object-oriented wrapper of the HHVM/Hack cURL extension.
 *
 *
 * @see https://php.net/manual/curl.setup.php
 */
class Curl
{
    // The HTTP authentication method(s) to use.

    /**
     * @var string Type AUTH_BASIC
     */
    const AUTH_BASIC = CURLAUTH_BASIC;

    /**
     * @var string Type AUTH_DIGEST
     */
    const AUTH_DIGEST = CURLAUTH_DIGEST;

    /**
     * @var string Type AUTH_GSSNEGOTIATE
     */
    const AUTH_GSSNEGOTIATE = CURLAUTH_GSSNEGOTIATE;

    /**
     * @var string Type AUTH_NTLM
     */
    const AUTH_NTLM = CURLAUTH_NTLM;

    /**
     * @var string Type AUTH_ANY
     */
    const AUTH_ANY = CURLAUTH_ANY;

    /**
     * @var string Type AUTH_ANYSAFE
     */
    const AUTH_ANYSAFE = CURLAUTH_ANYSAFE;

    /**
     * @var string The user agent name which is set when making a request
     */
    const USER_AGENT = 'HHVM/Hack HHCurl (+https://github.com/kubotak-is/hhcurl)';

    private array $_cookies = [];

    private array $_headers = [];

    /**
     * @var resource Contains the curl resource created by `curl_init()` function
     */
    public $curl;

    /**
     * @var booelan Whether an error occured or not
     */
    public bool $error = false;

    /**
     * @var int Contains the error code of the curren request, 0 means no error happend
     */
    public int $error_code = 0;

    /**
     * @var string If the curl request failed, the error message is contained
     */
    public ?string $error_message = null;

    /**
     * @var booelan Whether an error occured or not
     */
    public bool $curl_error = false;

    /**
     * @var int Contains the error code of the curren request, 0 means no error happend
     */
    public int $curl_error_code = 0;

    /**
     * @var string If the curl request failed, the error message is contained
     */
    public ?string $curl_error_message = null;

    /**
     * @var booelan Whether an error occured or not
     */
    public bool $http_error = false;

    /**
     * @var int Contains the error code of the curren request, 0 means no error happend
     */
    public int $http_status_code = 0;

    /**
     * @var string If the curl request failed, the error message is contained
     */
    public ?string $http_error_message = null;

    /**
     * @var string|array TBD (ensure type) Contains the request header informations
     */
    public $request_headers = null;

    /**
     * @var array TBD (ensure type) Contains the response header informations
     */
    public $response_headers = [];

    /**
     * @var string Contains the response from the curl request
     */
    public ?string $response = null;

    /**
     * @var int $timeout
     */
    private int $timeout = 5;

    /**
     * @var string $url
     */
    private string $url = '';

    /**
     * @var boolean Whether the current section of response headers is after 'HTTP/1.1 100 Continue'
     */
    protected $response_header_continue = false;

    /**
     * Constructor ensures the available curl extension is loaded.
     *
     * @throws \ErrorException
     */
    public function __construct()
    {
        if (!extension_loaded('curl')) {
            throw new \ErrorException('The cURL extensions is not loaded, make sure you have installed the cURL extension: https://php.net/manual/curl.setup.php');
        }

        $this->init();
    }

    /**
     * @return Curl
     */
    public function getInstance(): Curl
    {
        return new self();
    }

    // private methods

    /**
     * Initializer for the curl resource.
     *
     * Is called by the __construct() of the class or when the curl request is reseted.
     * @return self
     */
    private function init()
    {
        $this->curl = curl_init();
        $this->setUserAgent(self::USER_AGENT);
        $this->setOpt(CURLINFO_HEADER_OUT, true);
        $this->setOpt(CURLOPT_HEADER, false);
        $this->setOpt(CURLOPT_RETURNTRANSFER, true);
        $this->setOpt(CURLOPT_HEADERFUNCTION, [$this, 'addResponseHeaderLine']);
        $this->setTimeOut(5);
        return $this;
    }

    // protected methods

    protected async function curl_exce(): Awaitable<string>
    {
        $mh = curl_multi_init();
        curl_multi_add_handle($mh, $this->curl);
        $active = -1;

        do {
            $ret = curl_multi_exec($mh, $active);
        } while ($ret == CURLM_CALL_MULTI_PERFORM);

        while ($active && $ret == CURLM_OK) {
            $flag = await curl_multi_await($mh, 2.0);

            if ($flag === -1) {
                await \HH\Asio\usleep(100);
            }

            do {
                $ret = curl_multi_exec($mh, $active);
            } while ($ret == CURLM_CALL_MULTI_PERFORM);
        }

        $out = curl_multi_getcontent($this->curl);

        curl_multi_remove_handle($mh, $this->curl);
        curl_multi_close($mh);

        return (string) $out;
    }

    /**
     * Execute the curl request based on the respectiv settings.
     *
     * @return int Returns the error code for the current curl request
     */
    protected async function exec(): Awaitable
    {
        $this->response_headers   = [];
        $this->response           = await $this->curl_exce();
        $this->curl_error_code    = curl_errno($this->curl);
        $this->curl_error_message = curl_error($this->curl);
        $this->curl_error         = !($this->curl_error_code === 0);
        $this->http_status_code   = curl_getinfo($this->curl, CURLINFO_HTTP_CODE);
        $this->http_error         = in_array(floor($this->http_status_code / 100), [4, 5]);
        $this->error              = $this->curl_error || $this->http_error;
        $this->error_code         = $this->error ? ($this->curl_error ? $this->curl_error_code : $this->http_status_code) : 0;

        $ci = curl_getinfo($this->curl, CURLINFO_HEADER_OUT);
        if ($ci === false) {
            throw new HHCurlException("Failed Curl Get Info, [url] {$this->url}");
        }

        $this->request_headers    = preg_split('/\r\n/', $ci, null, PREG_SPLIT_NO_EMPTY);
        $this->http_error_message = $this->error ? (isset($this->response_headers['0']) ? $this->response_headers['0'] : '') : '';
        $this->error_message      = $this->curl_error ? $this->curl_error_message : $this->http_error_message;

        if (empty($this->response) && !$this->curl_error && $this->http_status_code === 0) {
            throw new TimeOutException("TimeOut Curl: {$this->timeout} sec, [url] {$this->url}");
        }

        curl_close($this->curl);

        return $this->error_code;
    }

    /**
     * Handle writing the response headers
     *
     * @param resource $curl The current curl resource
     * @param string $header_line A line from the list of response headers
     *
     * @return int Returns the length of the $header_line
     */
    public function addResponseHeaderLine($curl, $header_line)
    {
        $trimmed_header = trim($header_line, "\r\n");

        if ($trimmed_header === "") {
            $this->response_header_continue = false;
        } else if (strtolower($trimmed_header) === 'http/1.1 100 continue') {
            $this->response_header_continue = true;
        } else if (!$this->response_header_continue) {
            $this->response_headers[] = $trimmed_header;
        }
        
        return strlen($header_line);
    }


    /**
     * @param array|object|string $data
     */
    protected function preparePayload($data)
    {
        $this->setOpt(CURLOPT_POST, true);

        if (is_array($data) || is_object($data)) {
            $data = http_build_query($data);
        }

        $this->setOpt(CURLOPT_POSTFIELDS, $data);
    }

    /**
     * Set auth options for the current request.
     *
     * Available auth types are:
     *
     * + self::AUTH_BASIC
     * + self::AUTH_DIGEST
     * + self::AUTH_GSSNEGOTIATE
     * + self::AUTH_NTLM
     * + self::AUTH_ANY
     * + self::AUTH_ANYSAFE
     *
     * @param int $httpauth The type of authentication
     */
    protected function setHttpAuth($httpauth)
    {
        $this->setOpt(CURLOPT_HTTPAUTH, $httpauth);
    }

    /**
     * @param string $url
     * @param array $data = []
     */
    protected function setUrl(string $url, array $data = []): void
    {
        $this->url = $url;
        if (count($data) > 0) {
            $this->setOpt(CURLOPT_URL, $url.'?'.http_build_query($data));
        } else {
            $this->setOpt(CURLOPT_URL, $url);
        }
    }

    /**
     * @deprecated calling exec() directly is discouraged
     */
    public function _exec()
    {
        return \HH\Asio\join($this->exec());
    }

    /**
     * Make a get request with optional data.
     *
     * The get request has no body data, the data will be correctly added to the $url with the http_build_query() method.
     *
     * @param string $url  The url to make the get request for
     * @param array  $data Optional arguments who are part of the url
     * @return self
     */
    public async function get(string $url, array $data = []): Awaitable
    {
        $this->setUrl($url, $data);
        $this->setOpt(CURLOPT_HTTPGET, true);
        await $this->exec();
        return $this;
    }

    /**
     * Make a post request with optional post data.
     *
     * @param string $url  The url to make the get request
     * @param mixed  $data Post data to pass to the url
     * @return self
     */
    public async function post(string $url, mixed $data = []): Awaitable
    {
        $this->setUrl($url);
        $this->preparePayload($data);
        await $this->exec();
        return $this;
    }

    /**
     * Make a put request with optional data.
     *
     * The put request data can be either sent via payload or as get paramters of the string.
     *
     * @param string $url     The url to make the get request
     * @param mixed  $data    Optional data to pass to the $url
     * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
     * @return self
     */
    public async function put(string $url, mixed $data = [], bool $payload = false): Awaitable
    {
        if (!empty($data)) {
            if ($payload === false) {
                $url .= '?'.http_build_query($data);
            } else {
                $this->preparePayload($data);
            }
        }

        $this->setUrl($url);
        $this->setOpt(CURLOPT_CUSTOMREQUEST, 'PUT');
        await $this->exec();
        return $this;
    }

    /**
     * Make a patch request with optional data.
     *
     * The patch request data can be either sent via payload or as get paramters of the string.
     *
     * @param string $url     The url to make the get request
     * @param mixed  $data    Optional data to pass to the $url
     * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
     * @return self
     */
    public async function patch(string $url, mixed $data = [], bool $payload = false): Awaitable
    {
        if (!empty($data)) {
            if ($payload === false) {
                $url .= '?'.http_build_query($data);
            } else {
                $this->preparePayload($data);
            }
        }

        $this->setUrl($url);
        $this->setOpt(CURLOPT_CUSTOMREQUEST, 'PATCH');
        await $this->exec();
        return $this;
    }

    /**
     * Make a delete request with optional data.
     *
     * @param string $url     The url to make the delete request
     * @param mixed  $data    Optional data to pass to the $url
     * @param bool   $payload Whether the data should be transmitted trough payload or as get parameters of the string
     * @return self
     */
    public async function delete(string $url, mixed $data = [], bool $payload = false): Awaitable
    {
        if (!empty($data)) {
            if ($payload === false) {
                $url .= '?'.http_build_query($data);
            } else {
                $this->preparePayload($data);
            }
        }

        $this->setUrl($url);
        $this->setOpt(CURLOPT_CUSTOMREQUEST, 'DELETE');
        await $this->exec();
        return $this;
    }

    /**
     * Make a head request with optional data.
     *
     * The get request has no body data, the data will be correctly added to the $url with the http_build_query() method.
     *
     * @param string $url  The url to make the get request for
     * @param array  $data Optional arguments who are part of the url
     * @return self
     */
    public async function head(string $url, array $data = []): Awaitable
    {
        $this->setUrl($url, $data);
        $this->setOpt(CURLOPT_HTTPGET, true);
        $this->setOpt(CURLOPT_NOBODY, true);
        await $this->exec();
        return $this;
    }

    // setters

    /**
     * Pass basic auth data.
     *
     * If the the rquested url is secured by an httaccess basic auth mechanism you can use this method to provided the auth data.
     *
     * ```php
     * $curl = new Curl();
     * $curl->setBasicAuthentication('john', 'doe');
     * $curl->get('http://example.com/secure.php');
     * ```
     *
     * @param string $username The username for the authentification
     * @param string $password The password for the given username for the authentification
     * @return self
     */
    public function setBasicAuthentication(string $username, string $password)
    {
        $this->setHttpAuth(self::AUTH_BASIC);
        $this->setOpt(CURLOPT_USERPWD, $username.':'.$password);
        return $this;
    }

    /**
     * Provide optional header informations.
     *
     * In order to pass optional headers by key value pairing:
     *
     * ```php
     * $curl = new Curl();
     * $curl->setHeader('X-Requested-With', 'XMLHttpRequest');
     * $curl->get('http://example.com/request.php');
     * ```
     *
     * @param string $key   The header key
     * @param string $value The value for the given header key
     * @return self
     */
    public function setHeader(string $key, string $value)
    {
        $this->_headers[$key] = $key.': '.$value;
        $this->setOpt(CURLOPT_HTTPHEADER, array_values($this->_headers));
        return $this;
    }

    /**
     * Provide a User Agent.
     *
     * In order to provide you cusomtized user agent name you can use this method.
     *
     * ```php
     * $curl = new Curl();
     * $curl->setUserAgent('My John Doe Agent 1.0');
     * $curl->get('http://example.com/request.php');
     * ```
     *
     * @param string $useragent The name of the user agent to set for the current request
     * @return self
     */
    public function setUserAgent(string $useragent)
    {
        $this->setOpt(CURLOPT_USERAGENT, $useragent);
        return $this;
    }

    /**
     * Set the HTTP referer header.
     *
     * The $referer informations can help identify the requested client where the requested was made.
     *
     * @param string $referer An url to pass and will be set as referer header
     * @return self
     */
    public function setReferer($referer)
    {
        $this->setOpt(CURLOPT_REFERER, $referer);
        return $this;
    }

    /**
     * Set cUrl Time Out
     * 
     * @param ?int $secound
     * @return self
     */
    public function setTimeOut(int $secound = 5)
    {
        $this->timeout = $secound;
        $this->setOpt(CURLOPT_TIMEOUT, $this->timeout);
        return $this;
    }

    /**
     * Set contents of HTTP Cookie header.
     *
     * @param string $key   The name of the cookie
     * @param string $value The value for the provided cookie name
     * @return self
     */
    public function setCookie(string $key, string $value)
    {
        $this->_cookies[$key] = $value;
        $this->setOpt(CURLOPT_COOKIE, http_build_query($this->_cookies, '', '; '));
        return $this;
    }

    /**
     * Set customized curl options.
     *
     * To see a full list of options: http://php.net/curl_setopt
     *
     * @see http://php.net/curl_setopt
     *
     * @param int   $option The curl option constante e.g. `CURLOPT_AUTOREFERER`, `CURLOPT_COOKIESESSION`
     * @param mixed $value  The value to pass for the given $option
     */
    public function setOpt(int $option, $value)
    {
        return curl_setopt($this->curl, $option, $value);
    }

    /**
     * Get customized curl options.
     *
     * To see a full list of options: http://php.net/curl_getinfo
     *
     * @see http://php.net/curl_getinfo
     *
     * @param int   $option The curl option constante e.g. `CURLOPT_AUTOREFERER`, `CURLOPT_COOKIESESSION`
     * @param mixed $value  The value to check for the given $option
     */
    public function getOpt($option)
    {
        return curl_getinfo($this->curl, $option);
    }
    
     /**
     * Return the endpoint set for curl
     *
     * @see http://php.net/curl_getinfo
     *
     * @return string of endpoint
     */
    public function getEndpoint(): string
    {
        return $this->getOpt(CURLINFO_EFFECTIVE_URL);
    }

    /**
     * Enable verbositiy.
     *
     * @todo As to keep naming convention it should be renamed to `setVerbose()`
     *
     * @param string $on
     * @return self
     */
    public function verbose($on = true)
    {
        $this->setOpt(CURLOPT_VERBOSE, $on);
        return $this;
    }

    /**
     * Reset all curl options.
     *
     * In order to make multiple requests with the same curl object all settings requires to be reset.
     * @return self
     */
    public function reset()
    {
        $this->close();
        $this->_cookies = [];
        $this->_headers = [];
        $this->error = false;
        $this->error_code = 0;
        $this->error_message = null;
        $this->curl_error = false;
        $this->curl_error_code = 0;
        $this->curl_error_message = null;
        $this->http_error = false;
        $this->http_status_code = 0;
        $this->http_error_message = null;
        $this->request_headers = null;
        $this->response_headers = [];
        $this->response = null;
        $this->setTimeOut(5);
        $this->url = '';
        $this->init();
        return $this;
    }

    /**
     * Closing the current open curl resource.
     * @return self
     */
    public function close()
    {
        if (is_resource($this->curl)) {
            curl_close($this->curl);
        }
        return $this;
    }

    /**
     * Close the connection when the Curl object will be destroyed.
     */
    public function __destruct()
    {
        $this->close();
    }

    /**
     * Was an 'info' header returned.
     * @return bool
     */
    public function isInfo(): bool
    {
        return $this->http_status_code >= 100 && $this->http_status_code < 200;
    }

    /**
     * Was an 'OK' response returned.
     * @return bool
     */
    public function isSuccess(): bool
    {
        return $this->http_status_code >= 200 && $this->http_status_code < 300;
    }

    /**
     * Was a 'redirect' returned.
     * @return bool
     */
    public function isRedirect(): bool
    {
        return $this->http_status_code >= 300 && $this->http_status_code < 400;
    }

    /**
     * Was an 'error' returned (client error or server error).
     * @return bool
     */
    public function isError(): bool
    {
        return $this->http_status_code >= 400 && $this->http_status_code < 600;
    }

    /**
     * Was a 'client error' returned.
     * @return bool
     */
    public function isClientError(): bool
    {
        return $this->http_status_code >= 400 && $this->http_status_code < 500;
    }

    /**
     * Was a 'server error' returned.
     * @return bool
     */
    public function isServerError(): bool
    {
        return $this->http_status_code >= 500 && $this->http_status_code < 600;
    }
}
