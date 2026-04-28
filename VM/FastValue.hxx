#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <memory>
#include <string_view>
#include <string>

namespace pie::vm {
    namespace nan_utils {
        static constexpr std::uint64_t quiet_nan_v = 0x7ffc000000000000;
        static constexpr std::uint64_t positive_infinity_v = 0x7ff0000000000000; //! unsigned flag (0 << 63) | 0x0ff prefix
        static constexpr std::uint64_t negative_infinity_v = 0xfff0000000000000; //! signed flag (1 << 63) | 0x0ff prefix

        [[nodiscard]] constexpr double alias_special_ieee_double_v(std::uint64_t x) noexcept {
            static_assert(sizeof(double) == 8 && sizeof(std::uint64_t) == 8, "~ FastValue.hxx:17:\t pie::vm::nan_utils, requiring 64-bit support, is not supported on this architecture.")
            double d {};

            std::memcpy(&d, &x, sizeof(double));

            return d;
        }
    }

    enum class FastValueTag : std::uint8_t {
        invalid,
        num_boolean,    // ? C++ bool
        num_double,     // ? C++ double
        num_bigint,     // ? C++ bigint_type or std::ssize_t
        value_ref,      // ? FastValue*
        object_ref,     // ? polymorphic ObjectBase*
        string_ref,     // ? pointer to interned string
        // todo: figure out type information at runtime... maybe during compilation, these become preloaded ObjectBases. functions can have a vector of param type-name pairs in ORDER... causing distinct signatures for type names as per Type/Type.hxx --> "virtual Type base"!
        // todo: maybe implement sliced-views into bytecode which terminates after its dispatch, corresponding to a handle of old AST node contents:
        ///  EXAMPLE: `foo: Handle = x + 1` will view something like:
        ///  SLICE_BEGIN: 0x123: GET_ITEM 0x0 (string 'x')
        ///    0x124: PUSH_CONST 0x0 (int 1)
        ///    0x125: CALL_BUILT_IN 0x2 (
        ///      Intrinsic* -- bool(*cherry_builtin_add)(CherryState& == 0x...., int argc == 2))
        ///    )
        ///  SLICE_END: (terminate evaluation of this Syntax's code slice upon VM.IP == SLICE_BEGIN + SLICE_LEN)
        last
    };

    //? Forward declare state of Pie lang VM, named after the old saying: "as the cherry on top [of pies]" ;)
    //? See VM/ByteCode.ixx for more information.
    class CherryState;

    struct DudOpt {};

    class ObjectBase {
    public:
        enum class ContainerPolicy : std::uint8_t {
            str,    //? for strings
            listy,  //? for lists
            assoc   //? for objects
        };

        virtual ~ObjectBase() = default;

        virtual std::string get_typename(CherryState& state) const = 0;

        //? Use this for resolving original values behind references / accessing data in Any values e.g `x: Any = 1234, x = {1, 2, 3, 4};`
        virtual const FastValue* resolve(CherryState& state) const = 0; //? non-mutating overload here
        virtual FastValue* resolve(CherryState& state) = 0;

        //? If implemented, the VM state is affected, usually by preparing the stack offsets & call frame as additional state. Otherwise, return false for non-functional objects.
        virtual bool call(CherryState& state, int arg_count) = 0;

        //! NOTE: for every builtin specified, create a custom function for ObjectBase.
        
        virtual FastValue neg(CherryState& state) const = 0;
        virtual FastValue pow(CherryState& state, const Value& rhs) const = 0;
        virtual FastValue mul(CherryState& state, const Value& rhs) const = 0;
        virtual FastValue div(CherryState& state, const Value& rhs) const = 0;
        virtual FastValue mod(CherryState& state, const Value& rhs) const = 0;
        virtual FastValue add(CherryState& state, const Value& rhs) const = 0;
        virtual FastValue sub(CherryState& state, const Value& rhs) const = 0;

        //? Converts an ObjectBase to a bool via truthiness rules.
        virtual bool test(CherryState& state) const = 0;
        // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        virtual int to_int(CherryState& state) const = 0;
        virtual double to_double(CherryState& state) const = 0;
        virtual std::string to_string(CherryState& state) const = 0;

        virtual bool cmp_eq(CherryState& state, const Value& rhs) const = 0;
        virtual bool cmp_gt(CherryState& state, const Value& rhs) const = 0;
        virtual bool cmp_geq(CherryState& state, const Value& rhs) const = 0;
        virtual bool cmp_lt(CherryState& state, const Value& rhs) const = 0;
        virtual bool cmp_leq(CherryState& state, const Value& rhs) const = 0;

        virtual FastValue get_item(CherryState& state, const Value& key, ContainerPolicy policy) const = 0;

        //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
        virtual FastValue put_item(CherryState& state, const Value& key, const Value& item, ContainerPolicy policy) = 0;

        //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
        virtual FastValue del_item(CherryState& state, const Value& key, ContainerPolicy policy) = 0;

        // todo: figure out builtin_conditional semantics...
    };

    class FastValue {
    public:
    #ifdef WEB_PIE
        using bigint_type = bigint_type;
    #else
        using bigint_type = ssize_t;
    #endif

    using native_type = std::variant<DudOpt, bool, double, bigint_type, FastValue*, ObjectBase*, const std::string*>;

    private:
        native_type m_data;

    public:
        constexpr FastValue() noexcept
        : m_data {DudOpt {}} {}

        template <typename InitType> requires (std::is_constructible_v<native_type, std::remove_cvref_t<InitType>>)
        explicit constexpr FastValue(InitType&& arg)
        : m_data (std::forward<InitType>(arg)) {}

        [[nodiscard]] constexpr FastValueTag tag(this auto&& self) noexcept {
            if (const auto current_tag = static_cast<FastValueTag>(m_data.index()); current_tag != FastValueTag::value_ref) {
                return current_tag;
            }

            return std::get<FastValue*>(m_data)->tag();
        }

        [[nodiscard]] constexpr FastValue resolve() const noexcept {
            if (const auto self_tag = tag(); self_tag == FastValueTag::value_ref) {
                return std::get<FastValue*>(m_data)->resolve();
            }

            return FastValue {};
        }

        [[nodiscard]] constexpr const FastValue* as_value_ref() const noexcept {
            if (const auto self_tag = tag(); self_tag == FastValueTag::value_ref) {
                return std::get<const FastValue*>(m_data);
            }

            return nullptr;
        }

        [[nodiscard]] constexpr FastValue* as_value_ref() noexcept {
            if (const auto self_tag = tag(); self_tag == FastValueTag::value_ref) {
                return std::get<FastValue*>(m_data);
            }

            return nullptr;
        }

        [[nodiscard]] const ObjectBase* as_object() const noexcept {
            if (const auto self_tag = tag(); self_tag == FastValueTag::value_ref) {
                return std::get<const FastValue*>(m_data)->as_object();
            } else if (self_tag == FastValueTag::object_ref) {
                return std::get<const ObjectBase*>(m_data);
            }

            return nullptr;
        }

        [[nodiscard]] ObjectBase* as_object() noexcept {
            if (const auto self_tag = tag(); self_tag == FastValueTag::value_ref) {
                return std::get<FastValue*>(m_data)->as_object();
            } else if (self_tag == FastValueTag::object_ref) {
                return std::get<ObjectBase*>(m_data);
            }

            return nullptr;
        }

        [[nodiscard]] FastValue& neg(CherryState& state) const {
            const auto self_tag = tag();

            switch (self_tag) {
            case FastValueTag::num_boolean: std::get<bool>(m_data) = !std::get<bool>(m_data); break;
            case FastValueTag::num_double: std::get<double>(m_data) *= -1.0; break;
            case FastValueTag::num_bigint: std::get<bigint_type>(m_data) *= -1LL; break;
            case FastValueTag::value_ref: std::get<FastValue*>(m_data)->neg(state); break;
            case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->neg(state); break;
            case FastValueTag::invalid: default: m_data = {}; break;
            }

            return *this;
        }

        [[nodiscard]] FastValue& pow(CherryState& state, const Value& rhs) const {
            static const auto bigint_pow = [] (bigint_type base, bigint_type exp) constexpr noexcept {
                //! Disallow oversized pow's on BigInt... Excess powers will default to an invalid variant for m_data.
                static constinit int max_exp = (sizeof(decltype(base)) * 2);
                bigint_type answer = base;

                //? Exponent check here:
                if (exp >= max_exp) { return native_type {}; }

                for (int count = 0; count < max_exp; count++) {
                    answer *= base;
                }

                return native_type {answer};
            };

            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_double || rhs_tag == FastValueTag::num_double) {
                    m_data = std::pow(to_double(state), rhs.to_double(state));
                } else if (self_tag == FastValueTag::num_bigint || rhs_tag == FastValueTag::num_bigint) {
                    m_data = bigint_pow(to_int(state), rhs.to_int(state));
                } else {
                    m_data = {};
                }
            } else {   
                switch (self_tag) {
                    case FastValueTag::num_double: m_data = std::pow(std::get<double>(m_data), rhs.to_double(state)); break;
                    case FastValueTag::num_bigint: m_data = bigint_pow(to_int(state), rhs.to_int(state)); break;
                    case FastValueTag::value_ref: std::get<FastValue*>(m_data)->pow(state, rhs); break;
                    case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->pow(state, rhs); break;
                    case FastValueTag::invalid: default: m_data = {}; break;
                }
            }

            return *this;
        }

        [[nodiscard]] FastValue& mul(CherryState& state, const Value& rhs) const {
            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_double || rhs_tag == FastValueTag::num_double) {
                    m_data = to_double(state) * rhs.to_double(state);
                } else if (self_tag == FastValueTag::num_bigint || rhs_tag == FastValueTag::num_bigint) {
                    m_data = to_int(state) * rhs.to_int(state);
                } else {
                    m_data = {};
                }
            } else {   
                switch (self_tag) {
                    case FastValueTag::num_double: std::get<double>(m_data) *= rhs.to_double(state); break;
                    case FastValueTag::num_bigint: std::get<bigint_type>(m_data) *= rhs.to_int(state); break;
                    case FastValueTag::value_ref: std::get<FastValue*>(m_data)->mul(state, rhs); break;
                    case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->mul(state, rhs); break;
                    case FastValueTag::invalid: default: m_data = {}; break;
                }
            }

            return *this;
        }

        [[nodiscard]] FastValue& div(CherryState& state, const Value& rhs) const {
            static const auto non_fatal_double_div = [] (double quotient, double divisor) noexcept {
                using namespace nan_utils;

                static const double quiet_nan = alias_special_ieee_double_v(quiet_nan_v);
                static const double positive_infinity = alias_special_ieee_double_v(positive_infinity_v);
                static const double negative_infinity = alias_special_ieee_double_v(negative_infinity_v);

                if (quotient == 0.0 && divisor == 0.0) {
                    return quiet_nan;
                } else if (quotient > 0.0 && divisor == 0.0) {
                    return positive_infinity;
                } else if (quotient < 0.0 && divisor == 0.0) {
                    return negative_infinity;
                } else {
                    return quotient / divisor;
                }
            };

            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_double || rhs_tag == FastValueTag::num_double) {
                    m_data = non_fatal_double_div(to_double(state), rhs.to_double(state));
                } else {
                    m_data = {};
                }
            } else {   
                switch (self_tag) {
                    case FastValueTag::num_double: m_data = non_fatal_double_div(to_double(state), rhs.to_double(state)) break;
                    case FastValueTag::num_bigint: std::get<bigint_type>(m_data) /= rhs.to_int(state); break;
                    case FastValueTag::value_ref: std::get<FastValue*>(m_data)->div(state, rhs); break;
                    case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->div(state, rhs); break;
                    case FastValueTag::invalid: default: m_data = {}; break;
                }
            }

            return *this;
        }

        [[nodiscard]] FastValue& mod(CherryState& state, const Value& rhs) const {
            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_bigint || rhs_tag == FastValueTag::num_bigint) {
                    m_data = to_int(state) % rhs.to_int(state);
                } else {
                    m_data = {};
                }
            } else if (self_tag == FastValueTag::num_bigint) {
                //! Do validity check: modulo of 0 is basically counting divisions by 0LL which is invalid.
                if (const auto rhs_v = rhs.to_int(state); rhs_v != 0LL) {
                    std::get<bigint_type>(m_data) %= rhs_v;
                } else {
                    m_data = {};
                }
            } else {
                m_data = {};
            }

            return *this;
        }

        [[nodiscard]] FastValue& add(CherryState& state, const Value& rhs) const {
            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_double || rhs_tag == FastValueTag::num_double) {
                    m_data = to_double(state) + rhs.to_double(state);
                } else if (self_tag == FastValueTag::num_bigint || rhs_tag == FastValueTag::num_bigint) {
                    m_data = to_int(state) + rhs.to_int(state);
                } else {
                    m_data = {};
                }
            } else {   
                switch (self_tag) {
                    case FastValueTag::num_double: std::get<double>(m_data) += rhs.to_double(state); break;
                    case FastValueTag::num_bigint: std::get<bigint_type>(m_data) += rhs.to_int(state); break;
                    case FastValueTag::value_ref: std::get<FastValue*>(m_data)->add(state, rhs); break;
                    case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->add(state, rhs); break;
                    case FastValueTag::invalid: default: m_data = {}; break;
                }
            }

            return *this;
        }

        [[nodiscard]] FastValue& sub(CherryState& state, const Value& rhs) const {
            const auto self_tag = tag();
            const auto rhs_tag = rhs.tag();

            if (self_tag != rhs_tag) {
                if (self_tag == FastValueTag::num_double || rhs_tag == FastValueTag::num_double) {
                    m_data = to_double(state) - rhs.to_double(state);
                } else if (self_tag == FastValueTag::num_bigint || rhs_tag == FastValueTag::num_bigint) {
                    m_data = to_int(state) - rhs.to_int(state);
                } else {
                    m_data = {};
                }
            } else {   
                switch (self_tag) {
                    case FastValueTag::num_double: std::get<double>(m_data) -= rhs.to_double(state); break;
                    case FastValueTag::num_bigint: std::get<bigint_type>(m_data) -= rhs.to_int(state); break;
                    case FastValueTag::value_ref: std::get<FastValue*>(m_data)->sub(state, rhs); break;
                    case FastValueTag::object_ref: std::get<ObjectBase*>(m_data)->sub(state, rhs); break;
                    case FastValueTag::invalid: default: m_data = {}; break;
                }
            }

            return *this;
        }


        //? Converts an ObjectBase to a bool via truthiness rules.
        [[nodiscard]] bool test(CherryState& state) const {
            const auto self_tag = tag();

            switch (self_tag) {
            case FastValueTag::num_boolean: return std::get<bool>(m_data);
            // case FastValueTag::num_double: return static_cast<bigint_type>(std::get<double>(m_data)); // todo
            case FastValueTag::num_bigint: return std::get<bigint_type>(m_data) != 0LL;
            case FastValueTag::value_ref: return std::get<FastValue*>(m_data)->test(state);
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data) != nullptr;
            case FastValueTag::invalid: default: return false;
            }
        }

        //! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        //! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        [[nodiscard]] bigint_type to_int(CherryState& state) const {
            const auto self_tag = tag();

            switch (self_tag) {
            case FastValueTag::num_boolean: return std::get<bool>(m_data) ? 1 : 0;
            // case FastValueTag::num_double: return static_cast<bigint_type>(std::get<double>(m_data)); // todo
            case FastValueTag::num_bigint: return std::get<bigint_type>(m_data);
            case FastValueTag::value_ref:
                return std::get<FastValue*>(m_data)->to_int(state);
            case FastValueTag::object_ref:
                return std::get<ObjectBase*>(m_data)->to_int(state);
            case FastValueTag::invalid: default: return 0;
            }
        }

        [[nodiscard]] double to_double(CherryState& state) const {
            const auto self_tag = tag();

            switch (self_tag) {
            case FastValueTag::num_boolean: return std::get<bool>(m_data) ? 1.0 : 0.0;
            case FastValueTag::num_double: return std::get<double>(m_data);
            // case FastValueTag::num_bigint: return static_cast<double>(std::get<bigint_type>(m_data)); // todo
            case FastValueTag::value_ref:
                return std::get<FastValue*>(m_data)->to_double(state);
            case FastValueTag::object_ref:
                return std::get<ObjectBase*>(m_data)->to_double(state);
            case FastValueTag::invalid: default: return 0.0;
            }
        }

        [[nodiscard]] std::string to_string(CherryState& state) const {
            const auto self_tag = tag();

            switch (self_tag) {
            case FastValueTag::num_boolean: return std::to_string(std::get<bool>(m_data));
            case FastValueTag::num_double: return std::to_string(std::get<double>(m_data));
            case FastValueTag::num_bigint: return std::to_string(std::get<bigint_type>(m_data));
            case FastValueTag::value_ref:
                return std::get<FastValue*>(m_data)->to_string(state);
            case FastValueTag::object_ref:
                return std::get<ObjectBase*>(m_data)->to_string(state);
            case FastValueTag::invalid: default: return "(unknown)";
            }
        }


        [[nodiscard]] bool cmp_eq(CherryState& state, const Value& rhs) const {
            const auto real_rhs_v = rhs.resolve();
            const auto self_tag = tag();
            const auto rhs_tag = real_rhs_v.tag();

            if (self_tag != rhs_tag) {
                return false;
            }

            switch (self_tag) {
            case FastValueTag::num_boolean: return test(state) == real_rhs_v.test(state);
            case FastValueTag::num_double: return to_string(state) == real_rhs_v.to_string(state);
            case FastValueTag::num_bigint: return to_int(state) == real_rhs_v.to_int(state);
            case FastValueTag::value_ref: return resolve(state) == real_rhs_v;
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data)->cmp_eq(state, real_rhs_v);
            case FastValueTag::string_ref:
                return std::get<const std::string*>(m_data) == std::get<const std::string*>(real_rhs_v.m_data)
                    || std::get<const std::string*>(m_data)
                        ->operator==(*std::get<const std::string*>(real_rhs_v.m_data));
            case FastValueTag::invalid: default: return false;
            }
        }

        [[nodiscard]] bool cmp_gt(CherryState& state, const Value& rhs) const {
            const auto real_rhs_v = rhs.resolve();
            const auto self_tag = tag();
            const auto rhs_tag = real_rhs_v.tag();

            if (self_tag != rhs_tag) {
                return false;
            }

            switch (self_tag) {
            case FastValueTag::num_boolean: return test(state) > real_rhs_v.test(state);
            case FastValueTag::num_double: return to_string(state) > real_rhs_v.to_string(state);
            case FastValueTag::num_bigint: return to_int(state) > real_rhs_v.to_int(state);
            case FastValueTag::value_ref: return resolve(state) > real_rhs_v;
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data)->cmp_gt(state, real_rhs_v);
            case FastValueTag::invalid: default: return false;
            }
        }

        [[nodiscard]] bool cmp_geq(CherryState& state, const Value& rhs) const {
            const auto real_rhs_v = rhs.resolve();
            const auto self_tag = tag();
            const auto rhs_tag = real_rhs_v.tag();

            if (self_tag != rhs_tag) {
                return false;
            }

            switch (self_tag) {
            case FastValueTag::num_boolean: return test(state) >= real_rhs_v.test(state);
            case FastValueTag::num_double: return to_string(state) >= real_rhs_v.to_string(state);
            case FastValueTag::num_bigint: return to_int(state) >= real_rhs_v.to_int(state);
            case FastValueTag::value_ref: return resolve(state) >= real_rhs_v;
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data)->cmp_geq(state, real_rhs_v);
            case FastValueTag::invalid: default: return false;
            }
        }

        [[nodiscard]] bool cmp_lt(CherryState& state, const Value& rhs) const {
            const auto real_rhs_v = rhs.resolve();
            const auto self_tag = tag();
            const auto rhs_tag = real_rhs_v.tag();

            if (self_tag != rhs_tag) {
                return false;
            }

            switch (self_tag) {
            case FastValueTag::num_boolean: return test(state) < real_rhs_v.test(state);
            case FastValueTag::num_double: return to_string(state) < real_rhs_v.to_string(state);
            case FastValueTag::num_bigint: return to_int(state) < real_rhs_v.to_int(state);
            case FastValueTag::value_ref: return resolve(state) < real_rhs_v;
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data)->cmp_lt(state, real_rhs_v);
            case FastValueTag::invalid: default: return false;
            }
        }

        [[nodiscard]] bool cmp_leq(CherryState& state, const Value& rhs) const {
            const auto real_rhs_v = rhs.resolve();
            const auto self_tag = tag();
            const auto rhs_tag = real_rhs_v.tag();

            if (self_tag != rhs_tag) {
                return false;
            }

            switch (self_tag) {
            case FastValueTag::num_boolean: return test(state) <= real_rhs_v.test(state);
            case FastValueTag::num_double: return to_string(state) <= real_rhs_v.to_string(state);
            case FastValueTag::num_bigint: return to_int(state) <= real_rhs_v.to_int(state);
            case FastValueTag::value_ref: return resolve(state) <= real_rhs_v;
            case FastValueTag::object_ref: return std::get<ObjectBase*>(m_data)->cmp_lt(state, real_rhs_v);
            case FastValueTag::invalid: default: return false;
            }
        }

    };
}