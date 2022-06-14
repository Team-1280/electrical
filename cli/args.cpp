#include "args.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <ranges>
#include <fmt/color.h>

std::size_t Args::m_id_cnt = 0;

Args::Args(std::string&& name, std::string&& desc) : 
    m_name{std::move(name)},
    m_short_desc{std::move(desc)}
{
    this->m_id = this->m_id_cnt++;    
}

ArgId Args::arg(Arg &&arg) {
    this->m_args.push_back(std::move(arg));
    return ArgId {
        .idx = this->m_args.size() - 1,
        .parent = this->m_id
    };
}

ArgsId Args::command(Args &&command) {
    this->m_commands.push_back(std::move(command));
    return ArgsId {
        .idx = this->m_commands.size() - 1,
        .parent = this->m_id
    };
}

ArgMatches::ArgMatches(Args const& args) : m_args{args} {}

Optional<std::reference_wrapper<ArgMatch const>> ArgMatches::get(const ArgId arg) const {
    auto match = this->m_matches.find(arg.idx);
    if(match == this->m_matches.end()) {
        return {};
    } else {
        return std::cref(match->second);
    }
}

Optional<std::reference_wrapper<ArgMatches const>> ArgMatches::get_subcommand(const ArgsId command) const {
    if(!this->m_subcommand.has_value() || (*this->m_subcommand)->m_args.m_id != command.id) {
        return {};
    } else {
        return std::cref(**this->m_subcommand); 
    }
}

bool ArgMatches::add_opt(ArgId id, ArgMatch&& match) {
    if(id.parent != this->m_args.m_id) {
        if(this->m_subcommand.has_value()) {
            return (*this->m_subcommand)->add_opt(id, std::move(match));
        } else {
            return false;
        }
    } else {
        this->m_matches.insert_or_assign(id.idx, std::move(match));
        return true;
    }
}

//**MUST** be the same length as `write_arg`'s name is
static std::size_t name_len(Arg const& arg) {
    if(arg.short_name.has_value()) {
        if(arg.long_name.has_value()) {
            return fmt::formatted_size(
                "-{}, --{} {}",
                *arg.short_name,
                *arg.long_name,
                arg.arg_name.has_value() ? arg.arg_name->c_str() : ""
            );
        } else {
            return fmt::formatted_size("-{} {}", *arg.short_name, arg.arg_name.has_value() ? arg.arg_name->c_str() : "");
        }
    } else if(arg.long_name.has_value()) {
        return fmt::formatted_size("--{} {}", *arg.long_name, arg.arg_name.has_value() ? arg.arg_name->c_str() : "");
    }

    return 0;
}

static constexpr const char * BORDER_CHAR = "│";

static void write_arg(std::ostream& ostream, bool verbose, std::size_t longest_name, std::size_t space, Arg const& arg) {
    std::string name;
    if(arg.short_name.has_value()) {
        if(arg.long_name.has_value()) {
            name = fmt::format(
                "-{}, --{} {}",
                *arg.short_name,
                *arg.long_name,
                arg.arg_name.has_value() ? arg.arg_name->c_str() : ""
            );
        } else {
            name = fmt::format("-{} {}", *arg.short_name, arg.arg_name.has_value() ? arg.arg_name->c_str() : "");
        }
    } else if(arg.long_name.has_value()) {
        name = fmt::format("--{} {}", *arg.long_name, arg.arg_name.has_value() ? arg.arg_name->c_str() : "");
    }
    
    fmt::print(
        ostream,
        "{1:>{0}}  {3:>{2}}   {4}\n",
        space,
        BORDER_CHAR,
        longest_name,
        name,
        (verbose && arg.long_help.has_value()) ? *arg.long_help : arg.short_help
    );
}

void Args::print_help(std::ostream& ostream, bool verbose, std::size_t space) const {
    if(this->m_version.has_value()) {
        fmt::print(ostream, "{1:>{0}} {2} (v{3})\n", space, BORDER_CHAR, this->m_name, *this->m_version);
    } else {
        fmt::print(ostream, "{1:>{0}} {2}\n", space, BORDER_CHAR, this->m_name);
    }

    fmt::print(ostream, "{1:>{0}} {2}\n", space, BORDER_CHAR, (verbose && this->m_long_desc.has_value()) ? *this->m_long_desc : this->m_short_desc); 
    
    std::size_t longest = 0;
    auto longest_elem = std::max_element(
        this->m_args.begin(),
        this->m_args.end(),
        [](Arg const& lhs, Arg const& rhs) { return name_len(lhs) < name_len(rhs); }
    );
    if(longest_elem != this->m_args.end()) {
        longest = name_len(*longest_elem);
    }

    if(std::any_of(this->m_args.begin(), this->m_args.end(), [](Arg const& arg) { return !arg.takes_arg; })) {
        fmt::print(ostream, "{1:>{0}} [Flags]\n", space, BORDER_CHAR);
        for(const auto& arg : this->m_args) {
            if(arg.takes_arg) { continue; }
            write_arg(ostream, verbose, longest, space, arg); 
        }
    }
    if(std::any_of(this->m_args.begin(), this->m_args.end(), [](Arg const& arg) { return arg.takes_arg; })) {
        fmt::print(ostream, "{1:>{0}} [Options]\n", space, BORDER_CHAR);
        for(const auto& arg : this->m_args) {
            if(!arg.takes_arg) { continue; }
            write_arg(ostream, verbose, longest, space, arg);
        }
    }

    if(!this->m_commands.empty()) {
        fmt::print(ostream, "{1:>{0}} [Subcommands]\n", space, BORDER_CHAR);
        for(const auto& subcmd : this->m_commands) {
            subcmd.print_help(ostream, verbose, space + 5);
        }
    }
}

void Args::print_usage(std::ostream& ostream) {
    fmt::print(ostream, "Usage: {} ", this->m_name);
    if(std::any_of(this->m_args.begin(), this->m_args.end(), [](const auto& arg) { return !arg.arg_name.has_value() && arg.short_name.has_value(); })) {
        fmt::print(ostream, "[-");
        for(const auto& flag : this->m_args) {
            if(flag.arg_name.has_value() || !flag.short_name.has_value()) { continue; }
            fmt::print(ostream, "{}", *flag.short_name);
        }
        fmt::print(ostream, "] ");
    }
    for(const auto& opt : this->m_args) {
        if(!opt.arg_name.has_value() || !opt.short_name.has_value()) { continue; }
        fmt::print(ostream, "[-{} {}] ", *opt.short_name, *opt.arg_name);
    }
}

ArgMatches Args::matches(int argc, const char *argv[]) {
    ArgMatches root{*this};
    ArgMatches *tail{&root};

    //Populate all subcommands from root so we know what arguments should be parsed
    for(int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if(arg.length() < 1) continue;
        if(arg[0] == '-') {
            if(arg.length() >= 2 && arg[1] == '-') {
                if(arg.length() == 2) { break; }
                Optional<std::pair<Arg const&, ArgId>> opt{};
                Optional<std::string_view> optarg{};

                std::size_t eq = arg.find('=');
                if(eq != std::string_view::npos) {
                    std::string_view long_name = arg.substr(2, eq - 2);
                    auto opt_found = root.find_arg([long_name](Arg const& a) { return a.long_name == long_name; });
                    if(!opt_found.has_value()) {
                        throw std::runtime_error{fmt::format("Unknown command-line option {}", std::string{long_name})};
                    }
                    opt.emplace(*opt_found);
                    optarg = arg.substr(eq + 1);
                } else {
                    std::string_view long_name = arg.substr(2);
                    auto opt_found= root.find_arg([long_name](Arg const& a) { return a.long_name == long_name; });
                    if(!opt_found.has_value()) {
                        throw std::runtime_error{fmt::format("Unknown command-line option {}", std::string{long_name})};
                    }

                    if(opt_found->first.takes_arg) {
                        if(i + 1 < argc) {
                            i += 1;
                            optarg = std::string_view{argv[i]};
                        }
                    }

                    opt.emplace(*opt_found);
                }

                root.add_opt(
                    opt->second,
                    ArgMatch{ .arg = optarg, .long_name = true }
                );
            } else if(arg.length() > 1) {
                char first = arg.at(1);
                auto opt = root.find_arg([first](Arg const& a) { return a.short_name.has_value() && a.short_name == first; });
                if(!opt.has_value()) {
                    throw std::runtime_error{fmt::format("Short command-line option {} not found", first)};
                }

                if(opt->first.takes_arg) {
                    if(arg.length() > 2) {
                        root.add_opt(opt->second, ArgMatch { .arg = arg.substr(2), .long_name = false }); 
                        continue;
                    } else if(i + 1 < argc) {
                        i += 1;
                        root.add_opt(opt->second, ArgMatch { .arg{argv[i]}, .long_name = false }); 
                        continue;
                    }
                } //fallthrough if option takes an argument but none was found

                for(char opt_flag : arg.substr(1)) {
                    auto flag = root.find_arg([opt_flag](Arg const& a){ return a.short_name.has_value() && a.short_name == opt_flag;});
                    if(!flag.has_value()) {
                        throw std::runtime_error{fmt::format("Unknown short command-line option {}", opt_flag)};
                    }
                    root.add_opt(flag->second, ArgMatch{});
                }
            }
        } else {
            bool has_subcommand = false;
            for(const auto& subcommand : tail->m_args.m_commands) {
                if(subcommand.m_name == arg) {
                    tail->m_subcommand = std::unique_ptr<ArgMatches>{new ArgMatches{subcommand}};
                    tail = tail->m_subcommand->get();
                    has_subcommand = true;
                    break;
                }
            }

            if(!has_subcommand) {
                throw std::runtime_error{fmt::format("Unknown subcommand {}", std::string{arg})};
            }
        }
    }

    return root;
}
