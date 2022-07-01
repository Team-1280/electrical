#pragma once

#include "args.hpp"
#include "lib.hpp"

class BomCommand {
public:
    /** \brief Add this `subcommand` to the given `args` structure */
    BomCommand(Args& args);
    
    /** \brief Run this subcommand with a graph opened from an input file and parsed arguments */
    int run(BoardGraph& graph, ArgMatches const& args);
    
    /** \brief All output formats that can be passed to the `outfmt` command-line option */
    enum class OutputFmt {
        Text,
        Json,
    };
    
    /** \brief ID of this subcommand in whatever `Args` structure it has been added to */
    ArgsId id;
private:
    /** \brief Command-line output format option ID */
    ArgId m_outfmt_opt;
};
