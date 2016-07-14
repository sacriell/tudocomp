#ifndef _INCLUDED_DS_IPROVIDER_HPP
#define _INCLUDED_DS_IPROVIDER_HPP

namespace tudocomp {

//fwd
class SuffixArray;
class InverseSuffixArray;
class PhiArray;
class LCPArray;

class ITextDSProvider {

public:
    virtual const SuffixArray& require_sa() = 0;
    virtual const InverseSuffixArray& require_isa() = 0;
    virtual const PhiArray& require_phi() = 0;
    virtual const LCPArray& require_lcp() = 0;

    virtual std::unique_ptr<SuffixArray> release_sa() = 0;
    virtual std::unique_ptr<InverseSuffixArray> release_isa() = 0;
    virtual std::unique_ptr<PhiArray> release_phi() = 0;
    virtual std::unique_ptr<LCPArray> release_lcp() = 0;

    virtual uint8_t operator[](size_t) const = 0;
    virtual const uint8_t* text() const = 0;
    virtual size_t size() const = 0;
};

}

#endif

