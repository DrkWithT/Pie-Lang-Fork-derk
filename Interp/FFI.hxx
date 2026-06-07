#pragma once

#include <cstddef>

#include "../Value/Value.hxx"
#include "../Utils/utils.hxx"

#include <ffi.h>

inline namespace pie {
namespace ffi {

class FFI {
public:

    union {
        struct {
            value::Value  *value ;
            void *ptr;
        } single;

        struct {
            value::Value* *values;
            FFI* *nested;
            size_t n;
        } multi;
    };


    ffi_type *type;


    bool isStruct() const noexcept { return type->type == FFI_TYPE_STRUCT; }

    void print(const size_t indent = 0) const {
        std::string dent(indent, ' ');

        if (isStruct()) {
            std::clog << dent << "struct: " << std::endl;
            for (size_t i{}; i < multi.n; ++i) {
                std::clog << dent << "value: " << value::stringify(*multi.values[i]) << std::endl;
                (*multi.nested)->print(indent + 4);
            }
        }
        else {
            std::clog << dent << "regular value: " << value::stringify(*single.value) << std::endl;
        }
    }
};





inline FFI* prepareFFI(const value::Value& value, const BigInt type_id) {
    const auto value_ptr = new value::Value{value};


    switch (type_id) {
        case FFI_TYPE_INT: {
            if (not std::holds_alternative<BigInt>(*value_ptr)) util::error();
            return new FFI{value_ptr, &get<BigInt>(*value_ptr), &ffi_type_sint32};
        }

        case FFI_TYPE_POINTER: {
            if (not std::holds_alternative<std::string>(*value_ptr)) util::error();
            return new FFI{value_ptr, get<std::string>(*value_ptr).data(), &ffi_type_pointer};
        }

        case FFI_TYPE_STRUCT: {
            if (not std::holds_alternative<value::Object>(*value_ptr)) util::error();

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
            auto ret = new FFI{.multi = {new value::Value*[size], new ffi::FFI*[size], size}};

            // zip shortens the members list to the type list length.
            // this intended to allow the user to attach methods and other members to the Pie class
            for (size_t i{}; const auto& [id, member] : std::views::zip(c_types, obj.second->members)) {
                const auto& [name, _, value_ptr] = member;

                auto ffi = prepareFFI(*value_ptr, id);

                types[i] = ffi->type;
                ret->multi.nested[i] = ffi;
                ret->multi.values[i] = ffi->single.value;


                ++i;
            }


            const auto& ffi_struct = new ffi_type{{}, {}, FFI_TYPE_STRUCT, types};

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



inline void pack(std::byte* buffer, const FFI* ffi) {
    if (ffi->isStruct()) {
        size_t offset[256];
        ffi_get_struct_offsets(FFI_DEFAULT_ABI, ffi->type, offset);

        for (size_t i = 0; i < ffi->multi.n; ++i) {
            pack(buffer + offset[i], ffi->multi.nested[i]);
        }

        return;
    }

    switch (ffi->type->type) {
        case FFI_TYPE_SINT32:
        case FFI_TYPE_INT:
            *reinterpret_cast<int*>(buffer) = static_cast<int>(get<BigInt>(*ffi->single.value));
            return;

        case FFI_TYPE_POINTER:
            *reinterpret_cast<char**>(buffer) = get<std::string>(*ffi->single.value).data();
            return;
    }
}


} // namespace ffi
} // namespace pie
