#pragma once


#include <cstddef>

#include "../Value/Value.hxx"
#include "../Utils/utils.hxx"

#include <ffi.h>
#include <memory>
#include <ranges>

inline namespace pie {
namespace ffi {

class FFI {
public:

    struct Single {
        std::unique_ptr<value::Value> value;
        void *ptr = nullptr;
    };

    struct Multi {
        std::vector<std::unique_ptr<FFI>> nested;

        Multi() = default;
        Multi(const Multi&) = delete;
        Multi(Multi&&) = default;
        Multi& operator=(const Multi&) = delete;
        Multi& operator=(Multi&&) = default;
    };


    std::variant<Single, Multi> field;
    ffi_type *type;

    bool isStruct() const noexcept { return type->type == FFI_TYPE_STRUCT; }


    ~FFI() {
        if (isStruct()) {
            delete[] type->elements;
            delete type;
        }
    }
};



template <typename T>
static void errorCheck(const value::Value& value, const char* type_name) {
    if (not std::holds_alternative<T>(value))
        util::error("C Type indicated `" + (type_name + ("` bit value passed was: " + value::stringify(value))));
}


inline std::unique_ptr<FFI> prepareFFI(const value::Value& value, const BigInt type_id) {
    // const auto value_ptr = new auto{value};
    auto value_ptr = std::make_unique<value::Value>(value);

    switch (type_id) {
        case FFI_TYPE_SINT32:
        case FFI_TYPE_INT   : {
            errorCheck<BigInt>(value, "int");

            auto ret = std::make_unique<FFI>(
                FFI::Single{std::move(value_ptr)},
                &ffi_type_sint32
            );
            get<FFI::Single>(ret->field).ptr = &get<BigInt>(*get<FFI::Single>(ret->field).value);

            return ret;
        }

        case FFI_TYPE_UINT8: {
            errorCheck<BigInt>(value, "unsigned char");

            auto ret = std::make_unique<FFI>(
                FFI::Single{std::move(value_ptr)},
                &ffi_type_uchar
            );
            get<FFI::Single>(ret->field).ptr = &get<BigInt>(*get<FFI::Single>(ret->field).value);

            return ret;
        }

        case FFI_TYPE_POINTER: {
            errorCheck<std::string>(value, "pointer type");
            auto ret = std::make_unique<FFI>(
                FFI::Single{std::move(value_ptr)},
                &ffi_type_pointer
            );

            get<FFI::Single>(ret->field).ptr = get<std::string>(*get<FFI::Single>(ret->field).value).data();
            // get<std::string>(*value_ptr).data()

            return ret;
        }

        case FFI_TYPE_STRUCT: {
            // if (not std::holds_alternative<value::Object>(*value_ptr)) util::error();
            errorCheck<value::Object>(value, "struct");

            const auto& obj = get<value::Object>(*value_ptr);

            std::vector<BigInt> c_types;
            for (const auto& [name, _, value_ptr] : obj.second->members) {
                if (name.name == "__types") {
                    if (not std::holds_alternative<value::ListValue>(*value_ptr))
                        util::error("Special Member `__types` must be a list filled with C Types: " + value::stringify(value));

                    const auto& list = get<value::ListValue>(*value_ptr);
                    for (const auto& elt : list.elts->values) {
                        if (not std::holds_alternative<BigInt>(elt))
                            util::error("Special Member `__types` must be a list filled with C Types: " + value::stringify(value));

                        c_types.push_back(get<BigInt>(elt));
                    }
                }
            }


            const size_t size = c_types.size();
            const auto types = new ffi_type*[size + 1];
            types[size] = nullptr;
            auto ret = std::make_unique<FFI>(FFI::Multi{});

            // zip shortens the members list to the type list length.
            // this intended to allow the user to attach methods and other members to the Pie class

            size_t i{};
            for (
                auto& field = get<FFI::Multi>(ret->field);
                const auto& [id, member] :
                    std::views::zip(
                        c_types,
                        obj.second->members | std::views::filter([] (const auto& member) { return get<0>(member).name != "__types"; })
                    )
            ) {
                const auto& [name, _, value_ptr] = member;

                auto ffi = prepareFFI(*value_ptr, id);

                // field.values.push_back(std::move(get<FFI::Single>(ffi->field)).value);
                types[i++] = ffi->type;
                field.nested.push_back(std::move(ffi));
            }


            const auto ffi_struct = new ffi_type{{}, {}, FFI_TYPE_STRUCT, types};

            // necessary call to `ffi_get_struct_offsets` to fill in .size and .alignment of `ffi_struct`
            size_t dummy[256];
            ffi_get_struct_offsets(
                FFI_DEFAULT_ABI,
                ffi_struct,
                dummy
            );

            ret->type = ffi_struct;

            return ret;
        }
    }


    util::error();
}



inline void pack(std::byte *buffer, const FFI *ffi) {
    if (ffi->isStruct()) {
        size_t offset[256];
        ffi_get_struct_offsets(FFI_DEFAULT_ABI, ffi->type, offset);

        const auto& field = get<FFI::Multi>(ffi->field);
        for (size_t i = 0; i < field.nested.size(); ++i) {
            pack(buffer + offset[i], field.nested[i].get());
        }

        return;
    }

    switch (ffi->type->type) {
            *reinterpret_cast<int*>(buffer) = static_cast<int>(get<BigInt>(*get<FFI::Single>(ffi->field).value));
            return;

        case FFI_TYPE_SINT32:
        case FFI_TYPE_INT   :
            *reinterpret_cast<int*>(buffer) = static_cast<int>(get<BigInt>(*get<FFI::Single>(ffi->field).value));
            return;

        case FFI_TYPE_UINT8:
            *reinterpret_cast<unsigned char*>(buffer) = static_cast<unsigned char>(get<BigInt>(*get<FFI::Single>(ffi->field).value));
            return;

        case FFI_TYPE_POINTER:
            *reinterpret_cast<char**>(buffer) = get<std::string>(*get<FFI::Single>(ffi->field).value).data();
            return;
    }
}




} // namespace ffi
} // namespace pie
