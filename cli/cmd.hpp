#pragma once

#include <functional>

#include "args.hpp"
#include "fmt/core.h"
#include "lib.hpp"
#include "util/optional.hpp"
#include "util/stackvec.hpp"

/**
 * \brief Command containing a character prefix and function to run
 */
class Command {
public:
    /** \brief Get the name of this `Command` */
    inline constexpr char prefix() const noexcept { return this->m_prefix; }
    /** \brief Get the help message for this `Command` */
    inline constexpr Args const& args() const noexcept { return this->m_args; }

    Command() = delete;
    /** \brief Create a new `Command` with the given name */
    Command(char prefix, Args&& args) : m_prefix{prefix}, m_args{std::move(args)} {}
    
    /** \brief Set the list of subcommands for this `Command` */
    Command&& with_subcmds(std::vector<Command>&& subcmds) && {
        this->m_subcmds = std::move(subcmds);
        return std::move(*this);
    }

    /**
     * \brief Run this `Command`, with the user-passed subcommands and arguments
     * \param subcmds A string slice of the remaining sub command prefixes beyond this `Command`
     * \param args A list of argument string slices that the user passed to this command
     */
    void run(StackVec<std::string_view> const& args);
    
    /** \brief Get the subcommand of this `Command` with the given prefix */
    Optional<std::reference_wrapper<Command>> get_subcmd(char prefix) {
        auto found = std::find_if(
            this->m_subcmds.begin(),
            this->m_subcmds.end(),
            [prefix](auto const& cmd) { return cmd.prefix() == prefix; }
        );
        if(found == this->m_subcmds.end()) {
            return {};
        }

        return std::ref(*found);
    }

    Optional<std::reference_wrapper<Command>> get_subcmd(std::string_view const prefixes) {
        return prefixes.empty() ?
            std::ref(*this) :
            this
                ->get_subcmd(prefixes.at(0))
                .map([prefixes](auto cmd) { return cmd.get().get_subcmd(prefixes.substr(1)); })
                .flatten();
    }
    virtual ~Command() = default;
private:
    /** \brief Name of this `Command` */
    char m_prefix;
    /** \brief Command-line argument parsing */
    Args m_args;
    /** \brief List of all subcommands for this command */
    std::vector<Command> m_subcmds;
};
