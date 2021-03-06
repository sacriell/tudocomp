#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISAFromSA: public Algorithm, public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    inline static ds::InputRestrictions restrictions() {
        return ds::InputRestrictions {};
    }

    template<typename textds_t>
    inline ISAFromSA(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Require Suffix Array
        auto& sa = t.require_sa(cm);

        StatPhase::wrap("Construct ISA", [&]{
            // Allocate
            const size_t n = t.size();
            const size_t w = bits_for(n);
            set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : LEN_BITS));

            // Construct
            for(len_t i = 0; i < n; i++) {
                (*this)[sa[i]] = i;
            }

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });

        if(cm == CompressMode::delayed) compress();
    }

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress ISA", [this]{
            width(bits_for(size()));
            shrink_to_fit();

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });
    }
};

} //ns
