#include "args.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
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
    this->m_args.push_back(arg);
    return ArgId {
        .idx = this->m_args.size() - 1,
        .parent = this->m_id
    };
}


ArgMatches::ArgMatches(Args const& args) : m_args{args} {}

Optional<std::reference_wrapper<ArgMatch const>> ArgMatches::get(const ArgId arg) const {
    auto match = this->m_matches.find(arg.idx);
    if(match == this->m_matches.end() || arg.parent != this->m_args.m_id) {
        return this->m_subcommand.map([arg](auto& subcmd) { return subcmd->get(arg); }).flatten();
    } else {
        return std::cref(match->second);
    }
}

Optional<std::string_view const> ArgMatches::get_arg(const ArgId arg) const {
    return this->get(arg)
        .map([](std::reference_wrapper<ArgMatch const> match) { return match.get().arg; })
        .unwrap_or(Optional<std::string_view const>{});
}

bool ArgMatches::add_opt(ArgId id, ArgMatch&& match) {
    if(id.parent != this->m_args.m_id) {
        return false;
    } else {
        this->m_matches.insert_or_assign(id.idx, std::move(match));
        return true;
    }
}

//**MUST** be the same length as `write_arg`'s name is
static std::size_t name_len(Arg const& arg) {
    std::size_t len = 0;
    if(arg.short_name.has_value()) {
        len += fmt::formatted_size(
            "-{}",
            arg.short_name.unwrap_unchecked()
        );
    }
    if(arg.long_name.has_value()) {
        len += fmt::formatted_size(
            "{}--{}",
            arg.short_name.has_value() ? ", " : "",
            arg.long_name.unwrap_unchecked()
        );
    }
    if(arg.arg_name.has_value()) {
        len += fmt::formatted_size(
            " {}",
            arg.arg_name.unwrap_unchecked()
        );
    }

    return len;
}

static constexpr const char * BORDER_CHAR = "???";

static void write_arg(std::ostream& ostream, bool verbose, std::size_t longest_name, std::size_t space, Arg const& arg) {
    fmt::memory_buffer name{};
     if(arg.short_name.has_value()) {
        fmt::format_to(
            std::back_inserter(name),
            "-{}",
            arg.short_name.unwrap_unchecked()
        );
    }
    if(arg.long_name.has_value()) {
        fmt::format_to(
            std::back_inserter(name),
            "{}--{}",
            arg.short_name.has_value() ? ", " : "",
            arg.long_name.unwrap_unchecked()
        );
    }
    if(arg.arg_name.has_value()) {
        fmt::format_to(
            std::back_inserter(name),
            " {}",
            arg.arg_name.unwrap_unchecked()
        );
    }
  
    fmt::print(
        ostream,
        "{1:>{0}}  {3:>{2}}   {4}\n",
        space,
        BORDER_CHAR,
        longest_name,
        std::string_view(name.data(), name.size()),
        verbose ? arg.long_help.unwrap_or(arg.short_help) : arg.short_help
    );
}

void Args::print_help(std::ostream& ostream, bool verbose, std::size_t space) const {
    if(this->m_version.has_value()) {
        fmt::print(ostream, "{1:>{0}} {2} (v{3})\n", space, BORDER_CHAR, this->m_name, this->m_version.unwrap_unchecked());
    } else {
        fmt::print(ostream, "{1:>{0}} {2}\n", space, BORDER_CHAR, this->m_name);
    }

    fmt::print(ostream, "{1:>{0}} {2}\n", space, BORDER_CHAR, verbose ? this->m_long_desc.unwrap_or(this->m_short_desc) : this->m_short_desc); 
    
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
            subcmd.print_help(ostream, verbose, space + 3);
        }
    }
}

void Args::print_usage(std::ostream& ostream) const {
    fmt::print(ostream, "Usage: {} ", this->m_name);
    if(std::any_of(this->m_args.begin(), this->m_args.end(), [](const auto& arg) { return !arg.arg_name.has_value() && arg.short_name.has_value(); })) {
        fmt::print(ostream, "[-");
        for(const auto& flag : this->m_args) {
            if(flag.arg_name.has_value() || !flag.short_name.has_value()) { continue; }
            fmt::print(ostream, "{}", flag.short_name.unwrap_unchecked());
        }
        fmt::print(ostream, "] ");
    }
    for(const auto& opt : this->m_args) {
        if(!opt.arg_name.has_value() || !opt.short_name.has_value()) { continue; }
        fmt::print(ostream, "[-{} {}] ", opt.short_name.unwrap_unchecked(), opt.arg_name.unwrap_unchecked());
    }
}

ArgMatches Args::matches(int argc, const char *argv[]) {
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
                    auto const&& opt_found = root
                        .find_arg([long_name](Arg const& a) { return a.long_name == long_name; })
                        .unwrap_except(std::runtime_error{fmt::format("Unknown command-line option {}", std::string{long_name})});
                    opt.emplace(opt_found);
                    optarg = arg.substr(eq + 1);
                } else {
                    std::string_view long_name = arg.substr(2);
                    auto const&& opt_found = root
                        .find_arg([long_name](Arg const& a) { return a.long_name == long_name; })
                        .unwrap_except(std::runtime_error{fmt::format("Unknown command-line option {}", std::string{long_name})});
                    if(opt_found.first.takes_arg) {
                        if(i + 1 < argc) {
                            i += 1;
                            optarg = std::string_view{argv[i]};
                        }
                    }

                    opt.emplace(opt_found);
                }

                this->add_opt(
                    opt.unwrap_unchecked().second,
                    ArgMatch{ .arg = optarg, .long_name = true }
                );
            } else if(arg.length() > 1) {
                char first = arg.at(1);
                auto opt = root.find_arg([&first](Arg const& a) {
                    return a.short_name.map([&first](auto const& name) {
                        return name == first;
                    }).unwrap_or(false);
                })
                .unwrap_except(std::runtime_error{fmt::format("Short command-line option {} not found", first)});

                if(opt.first.takes_arg) {
                    if(arg.length() > 2) {
                        root.add_opt(opt.second, ArgMatch { .arg = arg.substr(2), .long_name = false }); 
                        continue;
                    } else if(i + 1 < argc) {
                        i += 1;
                        root.add_opt(opt.second, ArgMatch { .arg{argv[i]}, .long_name = false }); 
                        continue;
                    }
                } //fallthrough if option takes an argument but none was found

                for(char opt_flag : arg.substr(1)) {
                    auto flag = root
                        .find_arg([opt_flag](Arg const& a){ return a.short_name  == opt_flag;})
                        .unwrap_except(std::runtime_error{fmt::format("Unknown short command-line option {}", opt_flag)});
                    root.add_opt(flag.second, ArgMatch{});
                }
            }
        } else {
            throw std::runtime_error{fmt::format("Unknown subcommand {}", std::string{arg})};
        }
    }

    return root;
}
