//  supercollider-style synthdef
//  Copyright (C) 2008, 2009 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef SC_SYNTHDEF_HPP
#define SC_SYNTHDEF_HPP

#include <string>
#include <vector>
#include <map>

#include <boost/cstdint.hpp>
#include <boost/filesystem/path.hpp>

#include "SC_Types.h"
#include "SC_Wire.h"

namespace nova
{

class sc_synthdef
{
    typedef std::string string;
    typedef std::vector<float> fvector;
    typedef std::vector<string> svector;

    typedef boost::int16_t int16;
    typedef boost::int32_t int32;

public:
    typedef std::map<string, int> parameter_map_t;

    struct input_spec
    {
        input_spec(int16_t source, int16_t index):
            source(source), index(index)
        {}

        int16_t source;   /* index of ugen or -1 for constant */
        int16_t index;    /* number of output or constant index */
    };

    struct unit_spec_t
    {
        explicit unit_spec_t(const char *& buffer);

        unit_spec_t(string const & name, int16_t rate, int16_t special_index,
                    std::vector<input_spec> const & in_specs,
                    std::vector<char> const & out_specs):
            name(name), rate(rate), special_index(special_index),
            input_specs(in_specs), output_specs(out_specs)
        {}

        string name;
        int16_t rate;           /* 0: scalar rate, 1: buffer rate, 2: full rate, 3: demand rate */
        int16_t special_index;

        std::vector<input_spec> input_specs;
        std::vector<char> output_specs;      /* calculation rates */
        std::vector<int16_t> buffer_mapping;

        std::size_t memory_requirement(void)
        {
            return input_specs.size()  * (sizeof(Wire*) + sizeof(float*)) +
                   output_specs.size() * (sizeof(Wire*) + sizeof(float*)) +
                   output_specs.size() * sizeof(Wire);
        }

        struct sc_ugen_def * prototype;
    };

    friend class sc_synth_prototype;
    friend class sc_synth;
    friend class sc_ugen_factory;
    friend class sc_ugen_def;

    typedef std::vector<unit_spec_t> graph_t;
    typedef std::vector<int32_t> calc_units_t;

    explicit sc_synthdef(const char *& buffer);

    string dump(void) const;

    string const & name(void) const
    {
        return name_;
    }

    std::size_t parameter_count(void) const
    {
        return parameters.size();
    }

    std::size_t unit_count(void) const
    {
        return graph.size();
    }

    std::size_t calc_unit_count(void) const
    {
        return calc_unit_indices.size();
    }

    std::size_t memory_requirement(void) const
    {
        assert(memory_requirement_);
        return memory_requirement_;
    }

private:
    void read_synthdef(const char *&);

    /** assign buffers, collect memory requirement & cache ugen prototype */
    void prepare(void);

    string name_;
    fvector constants;
    fvector parameters;
    parameter_map_t parameter_map;

    graph_t graph;
    boost::uint16_t buffer_count;
    calc_units_t calc_unit_indices; /**< indices of the units, that need to be calculated */
    std::size_t memory_requirement_;
};

std::vector<sc_synthdef> read_synthdefs(const char * buf_ptr);
std::vector<sc_synthdef> read_synthdef_file(boost::filesystem::path const & filename);

} /* namespace nova */

#endif /* SC_SYNTHDEF_HPP */
