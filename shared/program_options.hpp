#ifndef GRAEHL__SHARED__PROGRAM_OPTIONS_HPP
#define GRAEHL__SHARED__PROGRAM_OPTIONS_HPP


#include <boost/program_options.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>

namespace graehl {

template <class Ostream>
struct any_printer  : public boost::function<void (Ostream &,boost::any const&)>
{
    typedef boost::function<void (Ostream &,boost::any const&)> F;
    
    template <class T>
        struct typed_print
        {
            void operator()(Ostream &o,boost::any const& t) const
            {
                o << *boost::any_cast<T const>(&t);
            }
        };

            template <class T>            
        static
        void typed_print_template(Ostream &o,boost::any const& t)
    {
        o << *boost::any_cast<T const>(&t);
    }
    
    any_printer() {}
    
    template <class T>
        explicit any_printer(T const* tag) : F(typed_print<T>()) {
    }
    
    template <class T>
        void set()
    {
        F f(typed_print<T>());
        swap(f);
    }
};


// have to wrap regular options_description and store our own tables because
// author didn't make enough stuff protected/public or add a virtual print
// method to value_semantic
template <class Ostream>
struct printable_options_description
    : public boost::program_options::options_description
{
    typedef printable_options_description<Ostream> self_type;
    typedef boost::program_options::options_description options_description;
    typedef boost::program_options::option_description option_description;
    typedef boost::shared_ptr<self_type> group_type;
    typedef std::vector<group_type > groups_type;
    
    struct printable_option 
    {
        typedef boost::shared_ptr<option_description> OD;
        OD od;
        any_printer<Ostream> print;
        bool in_group;
        
        std::string const& name() 
        { return od->long_name(); }
        
        std::string const& description()
        { return od->description(); }
        
        
        std::string const& vmkey() 
        {
            return od->key(name());
        }
        template <class T>
        printable_option(T *tag, OD const& od) : print(tag),od(od),in_group(false) {}
        printable_option() : in_group(false) {}
    };
    typedef std::vector<printable_option > options_type;
    
    printable_options_description(unsigned line_length = m_default_line_length) :
        options_description(line_length) {}
    
    printable_options_description(const std::string& caption,
                                  unsigned line_length = m_default_line_length)
        : options_description(caption,line_length), caption(caption) {}

    self_type &add_options()
    { return *this; }
    
    
    template <class T,class C>
    self_type &
    operator()(char const* name,
               boost::program_options::typed_value<T,C> *val,
               char const*description=NULL)
    {
        printable_option opt((T *)0,simple_add(name,val,description));
        pr_options.push_back(opt);
        return *this;
    }

    
    self_type&
    add(self_type const& desc) 
    {
        options_description::add(desc);
        groups.push_back(group_type(new self_type(desc)));
        for (typename options_type::const_iterator i=desc.pr_options.begin(),e=desc.pr_options.end();
             i!=e;++i) {
            pr_options.push_back(*i);
            pr_options.back().in_group=true;
        }
        return *this;
    }

    void print_option(Ostream &o,
                      printable_option &opt,
                      boost::program_options::variable_value const & var,
                      bool only_value=false) 
    {
        using namespace boost;
        using namespace boost::program_options;
        using namespace std;    
        string const& name=opt.name();
        if (!only_value) {
            if (var.defaulted())
                o << "#DEFAULTED# ";
            if (var.empty()) {
                o << "#EMPTY# "<<name;
                return;
            }
            o << name<<" = ";            
        }
        opt.print(o,var.value());
    }

    /*
    struct showing
    {
        bool default,empty,description,only_value,hierarchy;
define GRAEHL__SHOWING(name)\
        show const &show_ ##name (bool val=true) const { const_cast<showing*>(this)->name=val; }
        GRAEHL_SHOWING(default)
        GRAEHL_SHOWING(empty)
        GRAEHL_SHOWING(description)
    };
    */
    
    enum { SHOW_DEFAULTED=0x1
           , SHOW_EMPTY=0x2
           , SHOW_DESCRIPTION=0x4
           ,  SHOW_HIERARCHY=0x8
           ,  SHOW_ALL=0xFFFF
    };  
        
    void print(Ostream &o,
               boost::program_options::variables_map &vm,
               int show_flags=SHOW_DESCRIPTION & SHOW_DEFAULTED & SHOW_HIERARCHY)
    {
        const bool show_defaulted=show_flags & SHOW_DEFAULTED;
        const bool show_description=show_flags & SHOW_DESCRIPTION;
        const bool hierarchy=show_flags & SHOW_HIERARCHY;
        const bool show_empty=show_flags & SHOW_EMPTY;
        
        using namespace boost::program_options;
        using namespace std;
        o << "### " << caption << endl;
        for (typename options_type::iterator i=pr_options.begin(),e=pr_options.end();
             i!=e;++i) {
            printable_option & opt=*i;
            if (hierarchy and opt.in_group)
                continue;
            variable_value const & var=vm[opt.vmkey()];
            if (var.defaulted() && !show_defaulted)
                continue;        
            if (var.empty() && !show_empty)
                continue;
            if (show_description)
                o << "# " << opt.description() << endl;
            print_option(o,opt,var);
            o << endl;
        }
        o << endl;
        if (hierarchy)
            for (typename groups_type::iterator i=groups.begin(),e=groups.end();
                 i!=e;++i)
                (*i)->print(o,vm,show_flags);
    }
    
        
 private:
    groups_type groups;
    options_type pr_options;
    std::string caption;
    boost::shared_ptr<option_description>
    simple_add(const char* name,
               const boost::program_options::value_semantic* s,
               const char * description = NULL)
    {
        typedef option_description OD;
        boost::shared_ptr<OD> od(
            (description ? new OD(name,s,description) : new OD(name,s))
            );
        options_description::add(od);
        return od;
    }
};

/// parses arguments, then stores/notifies from opts->vm.  returns unparsed
/// options and positional arguments, but if not empty, throws exception unless
/// allow_unrecognized_positional is true
template <class Ostream>
std::vector<std::string>
handle_options(int argc,char *argv[],
               printable_options_description<Ostream> const& opts,
               boost::program_options::variables_map &vm,
               bool allow_unrecognized_positional=false,
               bool allow_unrecognized_opts=false)
{
    using namespace boost::program_options;
    using namespace std;
    command_line_parser cl(argc,argv);
    cl.options(opts);
    if (allow_unrecognized_opts)
        cl.allow_unregistered();
    parsed_options parsed=cl.run();
    vector<string> unparsed=collect_unrecognized(parsed.options, include_positional);
    if (!allow_unrecognized_positional) {
        if (!unparsed.empty())
            throw std::runtime_error("Unrecognized argument: "+unparsed.front());
    }
    store(parsed,vm);
    notify(vm);
    return unparsed;
}


} //graehl

#endif