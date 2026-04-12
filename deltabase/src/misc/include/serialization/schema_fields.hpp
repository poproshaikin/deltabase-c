#ifndef DELTABASE_SCHEMA_FIELDS_HPP
#define DELTABASE_SCHEMA_FIELDS_HPP

#include "serializer_base.hpp"

#include <functional>
#include <memory>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <utility>
#include <type_traits>
#include <vector>

namespace misc::serialization
{
    template <typename TObject, typename TMember>
    class MemberField final : public ISchemaField<TObject>
    {
        using MemberPtr = TMember TObject::*;
        using WriteFn = std::function<bool(const TMember&, WriteCursor&)>;
        using ReadFn = std::function<bool(ReadCursor&, TMember&)>;

        MemberPtr member_ptr_;
        WriteFn write_fn_;
        ReadFn read_fn_;

    public:
        MemberField(MemberPtr member_ptr, WriteFn write_fn, ReadFn read_fn)
            : member_ptr_(member_ptr), write_fn_(std::move(write_fn)), read_fn_(std::move(read_fn))
        {
        }

        bool
        write(const TObject& object, WriteCursor& cursor) const override
        {
            return write_fn_(object.*member_ptr_, cursor);
        }

        bool
        read(ReadCursor& cursor, TObject& object) const override
        {
            return read_fn_(cursor, object.*member_ptr_);
        }
    };

    template <typename TObject>
    class CustomField final : public ISchemaField<TObject>
    {
        using WriteFn = std::function<bool(const TObject&, WriteCursor&)>;
        using ReadFn = std::function<bool(ReadCursor&, TObject&)>;

        WriteFn write_fn_;
        ReadFn read_fn_;

    public:
        CustomField(WriteFn write_fn, ReadFn read_fn)
            : write_fn_(std::move(write_fn)), read_fn_(std::move(read_fn))
        {
        }

        bool
        write(const TObject& object, WriteCursor& cursor) const override
        {
            return write_fn_(object, cursor);
        }

        bool
        read(ReadCursor& cursor, TObject& object) const override
        {
            return read_fn_(cursor, object);
        }
    };

    template <typename TObject>
    class ConditionalField final : public ISchemaField<TObject>
    {
        using PredicateFn = std::function<bool(const TObject&)>;
        using ReadSkipFn = std::function<void(TObject&)>;

        PredicateFn predicate_;
        std::unique_ptr<ISchemaField<TObject>> field_;
        ReadSkipFn read_skip_fn_;

    public:
        ConditionalField(
            PredicateFn predicate,
            std::unique_ptr<ISchemaField<TObject>> field,
            ReadSkipFn read_skip_fn
        )
            : predicate_(std::move(predicate)), field_(std::move(field)),
              read_skip_fn_(std::move(read_skip_fn))
        {
        }

        bool
        write(const TObject& object, WriteCursor& cursor) const override
        {
            if (!predicate_(object))
                return true;

            return field_->write(object, cursor);
        }

        bool
        read(ReadCursor& cursor, TObject& object) const override
        {
            if (!predicate_(object))
            {
                if (read_skip_fn_)
                    read_skip_fn_(object);

                return true;
            }

            return field_->read(cursor, object);
        }
    };

    template <typename TObject, typename TVariant, typename TTrue, typename TFalse>
    class BoolVariantField final : public ISchemaField<TObject>
    {
        bool TObject::* discriminator_member_;
        TVariant TObject::* variant_member_;
        const Schema<TTrue>* true_schema_;
        const Schema<TFalse>* false_schema_;

    public:
        BoolVariantField(
            bool TObject::* discriminator_member,
            TVariant TObject::* variant_member,
            const Schema<TTrue>* true_schema,
            const Schema<TFalse>* false_schema
        )
            : discriminator_member_(discriminator_member), variant_member_(variant_member),
              true_schema_(true_schema), false_schema_(false_schema)
        {
        }

        bool
        write(const TObject& object, WriteCursor& cursor) const override
        {
            const bool discriminator = object.*discriminator_member_;
            const auto& variant_value = object.*variant_member_;

            if (discriminator)
                return write_by_schema(std::get<TTrue>(variant_value), *true_schema_, cursor);

            return write_by_schema(std::get<TFalse>(variant_value), *false_schema_, cursor);
        }

        bool
        read(ReadCursor& cursor, TObject& object) const override
        {
            const bool discriminator = object.*discriminator_member_;
            auto& variant_value = object.*variant_member_;

            if (discriminator)
            {
                TTrue true_value;
                if (!read_by_schema(cursor, *true_schema_, true_value))
                    return false;

                variant_value = std::move(true_value);
                return true;
            }

            TFalse false_value;
            if (!read_by_schema(cursor, *false_schema_, false_value))
                return false;

            variant_value = std::move(false_value);
            return true;
        }
    };

    template <typename TObject, typename TMember>
    std::unique_ptr<ISchemaField<TObject>>
    make_pod_field(TMember TObject::* member_ptr)
    {
        static_assert(std::is_trivially_copyable_v<TMember> || std::is_enum_v<TMember>);

        if constexpr (std::is_enum_v<TMember>)
        {
            using Underlying = std::underlying_type_t<TMember>;
            return std::make_unique<MemberField<TObject, TMember>>(
                member_ptr,
                [](const TMember& value, WriteCursor& cursor)
                {
                    const auto raw = static_cast<Underlying>(value);
                    return cursor.write_pod(raw);
                },
                [](ReadCursor& cursor, TMember& out)
                {
                    Underlying raw{};
                    if (!cursor.read_pod(raw))
                        return false;

                    out = static_cast<TMember>(raw);
                    return true;
                }
            );
        }
        else
        {
            return std::make_unique<MemberField<TObject, TMember>>(
                member_ptr,
                [](const TMember& value, WriteCursor& cursor)
                {
                    return cursor.write_pod(value);
                },
                [](ReadCursor& cursor, TMember& out)
                {
                    return cursor.read_pod(out);
                }
            );
        }
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_string_field(std::string TObject::* member_ptr)
    {
        return std::make_unique<MemberField<TObject, std::string>>(
            member_ptr,
            [](const std::string& value, WriteCursor& cursor)
            {
                return cursor.write_string(value);
            },
            [](ReadCursor& cursor, std::string& out)
            {
                return cursor.read_string(out);
            }
        );
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_uuid_field(types::UUID TObject::* member_ptr)
    {
        return std::make_unique<MemberField<TObject, types::UUID>>(
            member_ptr,
            [](const types::UUID& value, WriteCursor& cursor)
            {
                return cursor.write_uuid(value);
            },
            [](ReadCursor& cursor, types::UUID& out)
            {
                return cursor.read_uuid(out);
            }
        );
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_custom_field(
        std::function<bool(const TObject&, WriteCursor&)> write_fn,
        std::function<bool(ReadCursor&, TObject&)> read_fn
    )
    {
        return std::make_unique<CustomField<TObject>>(std::move(write_fn), std::move(read_fn));
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_conditional_field(
        std::function<bool(const TObject&)> predicate,
        std::unique_ptr<ISchemaField<TObject>> field,
        std::function<void(TObject&)> read_skip_fn = nullptr
    )
    {
        return std::make_unique<ConditionalField<TObject>>(
            std::move(predicate),
            std::move(field),
            std::move(read_skip_fn)
        );
    }

    template <typename TObject, typename TVariant, typename TTrue, typename TFalse>
    std::unique_ptr<ISchemaField<TObject>>
    make_bool_variant_field(
        bool TObject::* discriminator_member,
        TVariant TObject::* variant_member,
        const Schema<TTrue>* true_schema,
        const Schema<TFalse>* false_schema
    )
    {
        return std::make_unique<BoolVariantField<TObject, TVariant, TTrue, TFalse>>(
            discriminator_member,
            variant_member,
            true_schema,
            false_schema
        );
    }

    template <typename TObject, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_sized_vector_field(std::vector<TElement> TObject::* member_ptr)
    {
        static_assert(std::is_trivially_copyable_v<TElement>);

        return std::make_unique<MemberField<TObject, std::vector<TElement>>>(
            member_ptr,
            [](const std::vector<TElement>& values, WriteCursor& cursor)
            {
                const uint64_t count = values.size();
                if (!cursor.write_pod(count))
                    return false;

                if (count == 0)
                    return true;

                return cursor.write_bytes(values.data(), sizeof(TElement) * count);
            },
            [](ReadCursor& cursor, std::vector<TElement>& out)
            {
                uint64_t count = 0;
                if (!cursor.read_pod(count))
                    return false;

                out.resize(count);
                if (count == 0)
                    return true;

                return cursor.read_bytes(out.data(), sizeof(TElement) * count);
            }
        );
    }

    template <typename TObject, typename TCount, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_vector_count_field(
        TCount TObject::* count_member_ptr,
        std::vector<TElement> TObject::* values_member_ptr
    )
    {
        static_assert(std::is_integral_v<TCount>);

        return std::make_unique<CustomField<TObject>>(
            [values_member_ptr](const TObject& object, WriteCursor& cursor)
            {
                const auto count = static_cast<TCount>((object.*values_member_ptr).size());
                return cursor.write_pod(count);
            },
            [count_member_ptr](ReadCursor& cursor, TObject& object)
            {
                return cursor.read_pod(object.*count_member_ptr);
            }
        );
    }

    template <typename TObject, typename TCount, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_vector_object_payload_field(
        std::vector<TElement> TObject::* values_member_ptr,
        TCount TObject::* count_member_ptr,
        const Schema<TElement>* nested_schema
    )
    {
        static_assert(std::is_integral_v<TCount>);

        return std::make_unique<CustomField<TObject>>(
            [values_member_ptr, nested_schema](const TObject& object, WriteCursor& cursor)
            {
                for (const auto& value : object.*values_member_ptr)
                {
                    if (!write_by_schema(value, *nested_schema, cursor))
                        return false;
                }

                return true;
            },
            [values_member_ptr, count_member_ptr, nested_schema](ReadCursor& cursor, TObject& object)
            {
                const auto count = static_cast<uint64_t>(object.*count_member_ptr);
                auto& out = object.*values_member_ptr;
                out.clear();
                out.reserve(count);

                for (uint64_t index = 0; index < count; ++index)
                {
                    TElement value;
                    if (!read_by_schema(cursor, *nested_schema, value))
                        return false;

                    out.push_back(std::move(value));
                }

                return true;
            }
        );
    }

    template <typename TObject, typename TMember>
    std::unique_ptr<ISchemaField<TObject>>
    make_object_field(TMember TObject::* member_ptr, const Schema<TMember>* nested_schema)
    {
        return std::make_unique<MemberField<TObject, TMember>>(
            member_ptr,
            [nested_schema](const TMember& value, WriteCursor& cursor)
            {
                return write_by_schema(value, *nested_schema, cursor);
            },
            [nested_schema](ReadCursor& cursor, TMember& out)
            {
                return read_by_schema(cursor, *nested_schema, out);
            }
        );
    }

    template <typename TObject, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_vector_object_field(
        std::vector<TElement> TObject::* member_ptr,
        const Schema<TElement>* nested_schema
    )
    {
        return std::make_unique<MemberField<TObject, std::vector<TElement>>>(
            member_ptr,
            [nested_schema](const std::vector<TElement>& values, WriteCursor& cursor)
            {
                const uint64_t count = values.size();
                if (!cursor.write_pod(count))
                    return false;

                for (const auto& value : values)
                {
                    if (!write_by_schema(value, *nested_schema, cursor))
                        return false;
                }

                return true;
            },
            [nested_schema](ReadCursor& cursor, std::vector<TElement>& out)
            {
                uint64_t count = 0;
                if (!cursor.read_pod(count))
                    return false;

                out.clear();
                out.reserve(count);
                for (uint64_t index = 0; index < count; ++index)
                {
                    TElement value;
                    if (!read_by_schema(cursor, *nested_schema, value))
                        return false;
                    out.push_back(std::move(value));
                }

                return true;
            }
        );
    }

    template <typename TObject, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_vector_field(
        std::vector<TElement> TObject::* member_ptr,
        std::function<bool(const TElement&, WriteCursor&)> write_element,
        std::function<bool(ReadCursor&, TElement&)> read_element
    )
    {
        return std::make_unique<MemberField<TObject, std::vector<TElement>>>(
            member_ptr,
            [write_element = std::move(write_element)](
                const std::vector<TElement>& values,
                WriteCursor& cursor
            )
            {
                const uint64_t count = values.size();
                if (!cursor.write_pod(count))
                    return false;

                for (const auto& value : values)
                {
                    if (!write_element(value, cursor))
                        return false;
                }

                return true;
            },
            [read_element = std::move(read_element)](ReadCursor& cursor, std::vector<TElement>& out)
            {
                uint64_t count = 0;
                if (!cursor.read_pod(count))
                    return false;

                out.clear();
                out.reserve(count);
                for (uint64_t index = 0; index < count; ++index)
                {
                    TElement value;
                    if (!read_element(cursor, value))
                        return false;
                    out.push_back(std::move(value));
                }

                return true;
            }
        );
    }

    template <typename TObject, typename TElement>
    std::unique_ptr<ISchemaField<TObject>>
    make_vector_pod_field(std::vector<TElement> TObject::* member_ptr)
    {
        static_assert(std::is_trivially_copyable_v<TElement> || std::is_enum_v<TElement>);

        if constexpr (std::is_enum_v<TElement>)
        {
            using Underlying = std::underlying_type_t<TElement>;
            return make_vector_field<TObject, TElement>(
                member_ptr,
                [](const TElement& value, WriteCursor& cursor)
                {
                    const auto raw = static_cast<Underlying>(value);
                    return cursor.write_pod(raw);
                },
                [](ReadCursor& cursor, TElement& out)
                {
                    Underlying raw{};
                    if (!cursor.read_pod(raw))
                        return false;

                    out = static_cast<TElement>(raw);
                    return true;
                }
            );
        }

        return make_vector_field<TObject, TElement>(
            member_ptr,
            [](const TElement& value, WriteCursor& cursor)
            {
                return cursor.write_pod(value);
            },
            [](ReadCursor& cursor, TElement& out)
            {
                return cursor.read_pod(out);
            }
        );
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_optional_string_field(std::optional<std::string> TObject::* member_ptr)
    {
        return std::make_unique<MemberField<TObject, std::optional<std::string>>>(
            member_ptr,
            [](const std::optional<std::string>& value, WriteCursor& cursor)
            {
                if (!value.has_value())
                    return cursor.write_string("");

                return cursor.write_string(*value);
            },
            [](ReadCursor& cursor, std::optional<std::string>& out)
            {
                std::string value;
                if (!cursor.read_string(value))
                    return false;

                out.reset();
                out.emplace(std::move(value));
                return true;
            }
        );
    }

    template <typename TObject>
    std::unique_ptr<ISchemaField<TObject>>
    make_path_field(std::filesystem::path TObject::* member_ptr)
    {
        return std::make_unique<MemberField<TObject, std::filesystem::path>>(
            member_ptr,
            [](const std::filesystem::path& value, WriteCursor& cursor)
            {
                return cursor.write_string(value.string());
            },
            [](ReadCursor& cursor, std::filesystem::path& out)
            {
                std::string value;
                if (!cursor.read_string(value))
                    return false;

                out = value.c_str();
                return true;
            }
        );
    }
} // namespace misc::serialization

#endif // DELTABASE_SCHEMA_FIELDS_HPP
