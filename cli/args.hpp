#pragma once
#include "ser/store.hpp"
#include <ser/ser.hpp>
#include <stdexcept>
#include <iostream>

/** Identifier for a single `Arg` structure in an `Args` object */
struct ArgId {
    std::size_t idx;
    std::size_t parent;
};

/** Identifier for a single `Args` subcommand in an `Args` object */
struct ArgsId {
    std::size_t idx;
    std::size_t parent;
    std::size_t id;
};

class Args;

/**
 * \brief Structure with all data needed to generate help messages and parse command-line arguments
 */
struct Arg {
public:
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
};

/** 
 * \brief Structure constructed when the user passes an option on the command line, recording the argument (if any)
 */
struct ArgMatch {
    /** The argument value passed by the user, shared with the argv array */
    Optional<std::string_view> arg{};
};

class ArgMatches;

/**
 * \brief Class maintaining all command-line arguments, with utilities to parse them 
 * automatically from the main function's arguments
 */
class Args {
public:
    Args() = delete;

    /** 
     * \brief Create a new Args structure from all required fields 
     * \param name The name of the program or subcommand
     * \param desc A shortened description of the program's purpose
     */
    Args(std::string&& name, std::string&& desc);
    Args(Args&&) = default;
    
    /** 
     * \brief Register a command-line argument to be parsed
     * \param arg An argument structure with all data needed to parse the option
     * \return The ID of the argument, used to identify it in an `ArgMatches` structure
     */
    ArgId arg(Arg&& arg);

    /**
     * \brief Register a subcommand for this `Args` structure
     * \param command The subcommand structure that takes all sub arguments
     * \return ID of the added subcommand
     */
    ArgsId command(Args&& command); 
        
    /**
     * \brief Parse program arguments to an `ArgMatches` structure
     * \param argc Number of arguments in `argv`
     * \param argv Array of argument strings
     * \return An `ArgMatches` structure of parsed options
     */
    ArgMatches matches(int argc, const char *argv[]);
    
    /**
     * \brief Find an argument using a boolean predicate
     * \param p Predicate to apply to each argument 
     * \return An empty optional if `p` returns false for each argument, or the first argument that 
     * `p` returns true for
     */
    template<typename Predicate>
    requires requires(Predicate&& p, Arg const& arg) {
        {p(arg)} -> std::same_as<bool>;
    }
    Optional<ArgId> find_arg(Predicate&& p) {
        auto elem = std::find(
            this->m_args.begin(),
            this->m_args.end(),
            p
        );
        if(elem == this->m_args.cend()) {
            return {};
        } else {
            return *elem;
        }
    }
    
    /**
     * \brief Print a generated help message for this argument program to the given output stream
     * \param ostream Output stream to write a help message to
     * \param verbose If true, long descriptions and help messages will be printed
     */
    void print_help(std::ostream& ostream = std::cout, bool verbose = false, std::size_t space = 0) const;
private:
    /** Name of the program */
    std::string m_name;
    /** Short description of the program, used for the -h option */
    std::string m_short_desc;
    /** Longer description of the program, used for the --help option */
    Optional<std::string> m_long_desc;
    /** Version of the program for the -v option */
    Optional<std::string> m_version;
    /** \brief List of all registered command-line options */
    std::vector<Arg> m_args;
    /** \brief Sub programs of this arguments structure */
    std::vector<Args> m_commands;
    
    /** \brief Unique identifier for this Args structure, used to make processing argument matches less ambiguous */
    std::size_t m_id;

    /** \brief ID counter for generating unique argument IDs */
    static std::size_t m_id_cnt;

    friend class ArgMatches;
};

/** 
 * \brief Class maintaining all parsed argument matches after the `Args` class has finished parsing
 * the argv array
 */
class ArgMatches {
public:
    /** 
     * \brief Create a new empty ArgMatches structure using a reference to an `Args` structure
     * \param args Reference to an Args structure that must outlive this `ArgMatches` structure
     */
    ArgMatches(Args const& args);

    /** 
     * \brief Get a parsed argument match for the given argument
     * \param arg The ID of the argument to look up (get an argument ID from the `Args` class)
     * \return An empty Optional if there were no parsed options for the argument, or an `ArgMatch` structure with 
     * the parsed argument data
     */
    Optional<std::reference_wrapper<ArgMatch const>> get(const ArgId arg) const;
    
    /**
     * \brief Get subcommand argument matches, if the given subcommand was was passed
     * \param command ID of the subcommand
     * \return An empty Optional if the given subcommand was not passed, or argument matches for the given subcommand
     */
    Optional<std::reference_wrapper<ArgMatches const>> get_subcommand(const ArgsId command) const;
    
    /**
     * \brief Check if the given argument ID is present in this ArgMatches structure
     * \param arg The argument ID to check
     * \return true if the argument at the given ID was passed to the program 
     */
    inline bool has(const ArgId arg) const { return this->get(arg).has_value(); }
private:
    /** Map of argument indices to their parsed options */
    Map<std::size_t, ArgMatch> m_matches{};
    /** Passed subcommand matches, if any */
    Optional<std::unique_ptr<ArgMatches>> m_subcommand{}; 
    /** Reference to the arguments structure this represents */
    Args const& m_args;
        
    
    /**
     * \brief Add an option match to this `ArgMatches` structure or the subcommands of this structure
     * \param id The argument ID to insert a match into
     * \param match The argument match data to insert
     * \return true if the match was added to the given option
     */
    bool add_opt(ArgId id, ArgMatch&& match);
    
    /**
     * \brief Find an argument using the given predicate
     * \param p The predicate used to search this ArgMatches for a valid command line option in the current context
     * \return An empty optional if p returned false for each argument and each subcommand's arguments or the first argument that `p` returns true for
     */
    template<typename Predicate>
    requires requires(Predicate&& p, Arg const& arg) {
        {p(arg)} -> std::same_as<bool>;
    }
    Optional<std::pair<Arg const&, ArgId>> find_arg(Predicate&& p) {
        std::size_t idx = std::string::npos;
        Optional<std::reference_wrapper<Arg const>> found_arg{};
        for(std::size_t i = 0; const auto& arg : this->m_args.m_args) {
            if(p(arg)) {
                idx = i;
                found_arg = arg;
                break;
            }
        }
        if(idx == std::string::npos) {
            if(this->m_subcommand.has_value()) {
                return (*this->m_subcommand)->find_arg(p);
            } else {
                return {};
            }
        } else {
            return std::make_pair(*found_arg, ArgId { .idx = idx, .parent = this->m_args.m_id });
        }
    }

    friend class Args;
};