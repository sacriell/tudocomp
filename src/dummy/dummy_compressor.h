#ifndef DUMMY_COMPRESSOR_H
#define DUMMY_COMPRESSOR_H

#include "tudocomp.h"
#include "lz77rule.h"
using namespace lz77rule;

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace dummy {

using namespace tudocomp;

class DummyCompressor: public Lz77RuleCompressor {
public:
    inline DummyCompressor(Env& env): Lz77RuleCompressor(env) {}

    virtual Rules compress(Input& input, size_t threshold) final override;
};

}

#endif
