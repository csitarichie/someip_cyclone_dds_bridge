#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "adstutil_cxx/compiler_diagnostics.hpp"
ADST_DISABLE_CLANG_WARNING("unused-parameter")
#include <adstutil_cxx/static_string.hpp>
ADST_RESTORE_CLANG_WARNING()
#include "adstutil_cxx/error_handler.hpp"

struct yaml_parser_s;

using yaml_parser_t = yaml_parser_s;

namespace sstr = ak_toolkit::static_str;

namespace adst::common {

/**
 * Parse a YAML document and give access to the fields.
 */
class YamlWrapper
{
    // not copyable or movable
    YamlWrapper(const YamlWrapper&) = delete;
    YamlWrapper(YamlWrapper&&)      = delete;

    YamlWrapper& operator=(const YamlWrapper&) = delete;
    YamlWrapper& operator=(YamlWrapper&&) = delete;

public:
    /**
     * Creates Yaml object which provides access path key type of representation of the file.
     *
     * In case both static doc and file name is provided then file is read first if file does not exists or null
     * than static doc is used if both is NULL or file can not be read and static doc is NULL error is called.
     *
     * @param onErrorCallBack Error callback. This function will typically never return (e.g. by calling exit/abort).
     * @param staticDoc if it is not NULL and file can not be read or NULL then reads from the Static Doc.
     * @param fileName if it is not NULL wrapper reads from file if file exists.
     *
     */
    explicit YamlWrapper(const OnErrorCallBack& onErrorCallBack, const std::string& staticDoc,
                         const std::string& fileName);

    ~YamlWrapper() = default;

    /**
     * Getter for floating point values. The function has to be called with a specific type
     * template. In floating point case e.g. ''getValue<double>''.
     *
     * @param accessPath See details at getValueImpl.
     * @param separator See details at getValueImpl.
     * @return Pair of which the first is a valid/invalid bool flag and the second is the actual data.
     */
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, T>::type* = nullptr>
    std::pair<bool, T> getValue(const std::string& accessPath, const char separator = '/') const
    {
        char* id;
        auto  val = getValueImpl(accessPath, separator);
        T     ret = strtof(val, &id);
        return std::make_pair(*id == '\0', ret);
    }

    /**
     * Getter for signed integer (not bool) values. See above for details.
     */
    template <typename T,
              typename std::enable_if<
                  std::is_integral<T>::value && std::is_signed<T>::value && !std::is_same<T, bool>::value, T>::type = 0>
    std::pair<bool, T> getValue(const std::string& accessPath, const char separator = '/') const
    {
        char* id;
        auto  val = getValueImpl(accessPath, separator);
        T     ret = strtol(val, &id, 0);
        return std::make_pair(*id == '\0', ret);
    }

    /**
     * Getter for unsigned integer (not bool) values. See above for details.
     */
    template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value &&
                                                      !std::is_same<T, bool>::value,
                                                  T>::type = 0>
    std::pair<bool, T> getValue(const std::string& accessPath, const char separator = '/') const
    {
        char* id;
        auto  val = getValueImpl(accessPath, separator);
        T     ret = strtoul(val, &id, 0);
        return std::make_pair(*id == '\0', ret);
    }

    /**
     * Getter for boolean type values. Internally, this getter accepts true, True and 1 as true, and
     * false, False and 0 as false. See :cpp:func:`YamlWrapper::getValueImpl` for more details.
     */
    template <typename T, typename std::enable_if<std::is_same<T, bool>::value, T>::type = 0>
    std::pair<bool, T> getValue(const std::string& accessPath, const char separator = '/') const
    {
        auto value = std::string{getValueImpl(accessPath, separator)};
        if (value == "true" || value == "True" || value == "1") {
            return std::make_pair<bool, T>(true, true);
        }
        if (value == "false" || value == "False" || value == "0") {
            return std::make_pair<bool, T>(true, false);
        }
        return std::make_pair<bool, T>(false, false);
    }

    /**
     * Getter for string values. This function can be used for any value type. See above for
     * details.
     */
    template <typename T, typename = typename std::enable_if<std::is_same<T, std::string>::value>::type>
    std::pair<bool, const T> getValue(const std::string& accessPath, const char separator = '/') const
    {
        auto retStr = getValueImpl(accessPath, separator);
        return std::make_pair((retStr == std::string{NOT_FOUND}) ? false : true, std::string{retStr});
    }

private:
    /**
     * Abstract base class for every YAML node. All other nodes derive from this. Each node may have
     * a key which is used to identify the exact node. The key can be empty and a number too in case
     * of sequence elements.
     */
    class YamlNode
    {
    protected:
        /**
         * enum class for each types of nodes. The root node is always a DOC. Other nodes follow obviously.
         */
        enum class Type
        {
            VALUE,
            SEQUENCE,
            MAPPING,
            DOC
        };

    private:
        // not copyable or movable
        YamlNode(const YamlNode&) = delete;
        YamlNode(YamlNode&&)      = delete;

        YamlNode& operator=(const YamlNode&) = delete;
        YamlNode& operator=(YamlNode&&) = delete;

        /// The wrapper constuctor needs access to the fields.
        friend class YamlWrapper;

        const Type        type_;  /// The type of the node. See above
        const std::string value_; /// At scalars this is the real value, at other cases it's "<ClassName>Value"

    protected:
        /// The parent of the node in case of MAPPING and SEQUENCE type nodes.
        YamlNode& parent_;
        /// The children of the node if this is a MAPPING or a SEQUENCE type node.
        std::vector<std::unique_ptr<YamlNode>> children_;
        /// The name (key) of the node which can be used to identify the node.
        const std::string name_;

    public:
        /**
         * The more common constructor needs a parent node, a key, a type and a value.
         *
         * @param aParent Parent node.
         * @param aName Name (key) which will be used for identification.
         * @param aType Type of the node.
         * @param aValue Value of the node, which is most meaningful in case of scalars (leaf nodes).
         */
        explicit YamlNode(YamlNode& aParent, const std::string& aName, Type aType, const std::string& aValue)
            : type_(aType)
            , value_(aValue)
            , parent_(aParent)
            , name_(aName)
        {
        }

        /**
         * Construct a root node. Sets the type to DOC and the parent to
         * itself. This is because the root node is the parent of itself (we
         * cannot have a reference to NULL).
         */
        explicit YamlNode()
            : type_(Type::DOC)
            , value_("YamlDocValue")
            , parent_(*this)
        {
        }

        /**
         * Getter function for the node key.
         *
         * @return Node key.
         */
        const std::string& getKey() const
        {
            return name_;
        }

        /**
         * Getter function for the node type.
         *
         * @return Node type.
         */
        Type getType() const
        {
            return type_;
        }

        /**
         * Getter function for the the node value.
         *
         * @return the value of the node
         */
        const std::string& getValue() const
        {
            return value_;
        }

        virtual ~YamlNode() = 0;
    }; // class YamlNode

    /**
     * Concrete class for the doc (root) node. Shall not be used for other node types.
     */
    class YamlDoc : public YamlNode
    {
    public:
        explicit YamlDoc()
            : YamlNode()
        {
        }
    };

    /**
     * The mapping node. This is not an actual std::map but represents a YAML mapping. A mapping
     * node's children can be VALUE, MAPPING or SEQUENCE type nodes. Searching on it takes O(n)
     * time, where n is the number of elements.
     */
    class YamlMapping : public YamlNode
    {
    public:
        explicit YamlMapping(YamlNode& aParent, const std::string& aName)
            : YamlNode(aParent, aName, YamlNode::Type::MAPPING, "YamlMappingValue")
        {
        }
    };

    /**
     * The sequence node. It's child nodes are stored in order and can be VALUE, MAPPING and
     * SEQUENCE type nodes.
     */
    class YamlSequence : public YamlNode
    {
    public:
        explicit YamlSequence(YamlNode& aParent, const std::string& aName)
            : YamlNode(aParent, aName, YamlNode::Type::SEQUENCE, "YamlSequenceValue")
        {
        }
    };

    /**
     * The value node. Actual VALUEs are stored in this type of nodes.
     */
    class YamlValue : public YamlNode
    {
    public:
        explicit YamlValue(YamlNode& aParent, const std::string& aName, const std::string& aValue)
            : YamlNode(aParent, aName, YamlNode::Type::VALUE, aValue)
        {
        }
    };

    /**
     * Retrieves node value as string from the node tree. This is the core functionality for
     * accessing node values (since internally all node values are represented as strings).
     *
     * @param accessPath Access path to the node. Nested maps should be searched with their
     *        respective keys, sequences should be searched by index.
     * @param separator Separator which is used between access path elements for nested node
     *        searching.
     */
    const char* getValueImpl(const std::string& accessPath, char separator) const;

    /**
     * Parse the YAML doc from file and create the node tree.
     * @param fileName path to the file to read.
     * @return true in case of success otherwise false
     */
    bool initFromFile(const std::string& fileName);

    /**
     * Parse the YAML doc from static 0 terminated c string and create the node tree.
     * @param builtInDoc the compiled in config doc useful for tests where no extra file is provided.
     * @return true in case of success otherwise false
     */
    bool initFromConstString(const std::string& builtInDoc);

    bool parse(yaml_parser_t* parser);

    /// The root node. All other nodes are children of this node.
    std::unique_ptr<YamlNode> root_ = std::make_unique<YamlDoc>();
    /// Used as node value in case the actual node cannot be found.
    static constexpr auto NOT_FOUND = sstr::literal("Not Found");
    /// Default error callback function.
    const OnErrorCallBack& onErrorCallBack_;
};

inline YamlWrapper::YamlNode::~YamlNode()
{
}

} // namespace adst::common
