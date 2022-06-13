#include "args.hpp"
#include <functional>
#include <stdexcept>

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

void Args::print_help(std::ostream& ostream, bool verbose) const {
    ostream << this->m_name;
    if(this->m_version.has_value()) {
        ostream << " (" << *this->m_version << ")" << std::endl;
    } else {
        ostream << std::endl;
    }

    if(verbose) {
        ostream << this->m_long_desc.has_value() ? 
    }
}

ArgMatches Args::matches(int argc, const char *argv[]) {
    ArgMatches root{*this};
    ArgMatches *tail{&root};

    //Populate all subcommands from root so we know what arguments should be parsed
    for(int i = 0; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if(arg.length() == 0) continue;
        if(arg[0] == '-') {
            if(arg.length() >= 2 && argv[i][1] == '-') {
                if(arg.length() == 2) { break; }
                Optional<std::pair<Arg const&, ArgId>> opt{};
                Optional<std::string_view> optarg{};

                std::size_t eq = arg.find('=');
                if(eq != std::string_view::npos) {
                    std::string_view long_name = arg.substr(2, eq);
                    auto opt_found= root.find_arg([long_name](Arg const& a) { return a.long_name == long_name; });
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
                            optarg = std::string_view{argv[i + 1]};
                        }
                    }

                    opt.emplace(*opt_found);
                }

                root.add_opt(
                    opt->second,
                    ArgMatch{.arg = optarg}
                );
            } else if(arg.length() >= 1) {
                char first = arg.at(1);
                auto opt = root.find_arg([first](Arg const& a) { return a.short_name.has_value() && a.short_name == first; });
                if(!opt.has_value()) {
                    throw std::runtime_error{fmt::format("Short command-line option {} not found", first)};
                }

                if(opt->first.takes_arg) {
                    if(arg.length() > 2) {
                        root.add_opt(opt->second, ArgMatch {.arg = arg.substr(2)}); 
                        continue;
                    } else if(i + 1 < argc) {
                        root.add_opt(opt->second, ArgMatch {.arg{argv[i + 1]}}); 
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
            for(const auto& subcommand : tail->m_args.m_commands) {
                if(subcommand.m_name == arg) {
                    tail->m_subcommand = std::unique_ptr<ArgMatches>{new ArgMatches{subcommand}};
                    tail = tail->m_subcommand->get();
                }
            }
        }
    }

    return root;
}
