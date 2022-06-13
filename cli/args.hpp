#pragma once
#include "ser/store.hpp"
#include <ser/ser.hpp>

/**
 * \brief Structure with all data needed to generate help messages and parse command-line arguments
 */
struct Arg {
public:
    using Id = std::size_t;

    /** \brief If this option requires an argument to follow it */
    bool takes_arg = false;
    /** \brief Single character used to give the command line option following a single dash character */
    Optional<char> short_name{};
    /** \brief Long name used after two dash characters */
    Optional<std::string> long_name{}; 
    /** \brief Shortened help text to show when the user passes the special -h option */
    std::string short_help{};
    /** \brief More verbose help message shown when the user passes the --help option */
    Optional<std::string> long_help{};

    Arg(Arg&& other) = default;
};

/** 
 * \brief Structure constructed when the user passes an option on the command line, recording the argument (if any)
 */
struct ArgMatch {
    /** The argument value passed by the user, shared with the argv array */
    Optional<std::string_view> arg{};
};

/** 
 * \brief Class maintaining all parsed argument matches after the `Args` class has finished parsing
 * the argv array
 */
class ArgMatches {
public:
    /** 
     * \brief Get a parsed argument match for the given argument
     * \param arg The ID of the argument to look up (get an argument ID from the `Args` class)
     * \return An empty Optional if there were no parsed options for the argument, or an `ArgMatch` structure with 
     * the parsed argument data
     */
    Optional<ArgMatch const&> get(const Arg::Id arg) const;
private:
    /** Map of argument IDs to their parsed options */
    Map<Arg::Id, ArgMatch> m_matches;

    friend class Args;
};

/**
 * \brief Class maintaining all command-line arguments, with utilities to parse them 
 * automatically from the main function's arguments
 */
class Args {
public:
    Args() = default;
    
    /** 
     * \brief Register a command-line argument to be parsed
     * \param arg An argument structure with all data needed to parse the option
     * \return The ID of the argument, used to identify it in an `ArgMatches` structure
     */
    Arg::Id arg(Arg&& arg);
    
    /**
     * \brief Argument builder structure for a more 
     */
    struct Builder {

    };

private:
    /** \brief List of all registered command-line options */
    std::vector<Arg> m_args;
};


