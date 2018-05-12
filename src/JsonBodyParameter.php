<?hh // strict

namespace HHCurl;

class JsonBodyParameter extends AbstractParameter {
  public function __toString(): string {
    return \json_encode($this->values->toArray());
  }
}
