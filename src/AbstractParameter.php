<?hh // strict

namespace HHCurl;

<<__ConsistentConstruct>>
abstract class AbstractParameter {
  
  public function __construct(
    protected Map<mixed, mixed> $values
  ) {}

  abstract public function __toString(): string;
}
