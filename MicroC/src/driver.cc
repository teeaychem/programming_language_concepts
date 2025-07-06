#include "driver.hh"
#include "parser.hh"

Driver::Driver()
    : trace_parsing(false), trace_scanning(false) {
}

int Driver::parse(const std::string &f) {
  file = f;
  location.initialize(&file);
  int res;
  {
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    res = parse();
    scan_end();
  }
  return res;
}
