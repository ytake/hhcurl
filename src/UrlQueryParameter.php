<?hh // strict

namespace HHCurl;

class UrlQueryParameter extends AbstractParameter {
  public function __toString(): string {
    return \http_build_query($this->values->toArray());
  }
}
