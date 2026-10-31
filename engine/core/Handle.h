#pragma once

#include "engine/core/Assert.h"
#include "engine/core/Types.h"

#include <functional>
#include <optional>
#include <vector>

namespace br {

// Strongly typed generational handle. The Tag parameter prevents mixing e.g.
// mesh handles with light handles at compile time.
template <typename Tag>
struct Handle {
    u32 index = 0xFFFFFFFFu;
    u32 generation = 0;

    constexpr bool valid() const { return index != 0xFFFFFFFFu; }
    constexpr explicit operator bool() const { return valid(); }
    friend constexpr bool operator==(const Handle&, const Handle&) = default;
};

// Dense-ish slot map with generational validation. Lookups are O(1); stale
// handles are detected instead of silently aliasing a newly created object.
template <typename Tag, typename T>
class SlotMap {
public:
    using HandleT = Handle<Tag>;

    HandleT insert(T value) {
        u32 index;
        if (!m_freeList.empty()) {
            index = m_freeList.back();
            m_freeList.pop_back();
        } else {
            index = static_cast<u32>(m_slots.size());
            m_slots.emplace_back();
        }
        Slot& s = m_slots[index];
        s.value.emplace(std::move(value));
        ++m_count;
        return HandleT{index, s.generation};
    }

    bool erase(HandleT h) {
        if (!contains(h)) {
            return false;
        }
        Slot& s = m_slots[h.index];
        s.value.reset();
        ++s.generation;
        m_freeList.push_back(h.index);
        --m_count;
        return true;
    }

    bool contains(HandleT h) const {
        return h.index < m_slots.size() && m_slots[h.index].generation == h.generation &&
               m_slots[h.index].value.has_value();
    }

    T* get(HandleT h) { return contains(h) ? &*m_slots[h.index].value : nullptr; }
    const T* get(HandleT h) const { return contains(h) ? &*m_slots[h.index].value : nullptr; }

    usize size() const { return m_count; }

    template <typename F>
    void forEach(F&& fn) {
        for (u32 i = 0; i < m_slots.size(); ++i) {
            if (m_slots[i].value) {
                fn(HandleT{i, m_slots[i].generation}, *m_slots[i].value);
            }
        }
    }

    template <typename F>
    void forEach(F&& fn) const {
        for (u32 i = 0; i < m_slots.size(); ++i) {
            if (m_slots[i].value) {
                fn(HandleT{i, m_slots[i].generation}, *m_slots[i].value);
            }
        }
    }

    void clear() {
        for (u32 i = 0; i < m_slots.size(); ++i) {
            if (m_slots[i].value) {
                m_slots[i].value.reset();
                ++m_slots[i].generation;
                m_freeList.push_back(i);
            }
        }
        m_count = 0;
    }

private:
    struct Slot {
        std::optional<T> value;
        u32 generation = 0;
    };
    std::vector<Slot> m_slots;
    std::vector<u32> m_freeList;
    usize m_count = 0;
};

} // namespace br

template <typename Tag>
struct std::hash<br::Handle<Tag>> {
    size_t operator()(const br::Handle<Tag>& h) const noexcept {
        return std::hash<br::u64>{}((static_cast<br::u64>(h.generation) << 32) | h.index);
    }
};
