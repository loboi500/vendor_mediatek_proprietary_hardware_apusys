#ifndef _EDMA_MEMORY_BINDINGS_H_
#define _EDMA_MEMORY_BINDINGS_H_

#include <vector>

class EdmaMemoryBindings final {
public:
    explicit EdmaMemoryBindings(const size_t capacity) {
        mNormalAddrOffsets.reserve(capacity);
        mHighAddrOffsets.reserve(capacity);
        mOffsetInObjects.reserve(capacity);
    }

    ~EdmaMemoryBindings() = default;

public:
    void EnableHighAddr(bool enable = true) {
        if (enable) {
            mHeader |= 0x1;
        } else {
            mHeader &= (~0x1);
        }
    }

    bool HasHighAddr() const {
        return (mHeader & 0x1);
    }

public:
    void AppendNormal(const uint32_t offset, const uint32_t offsetInObject) {
        mNormalAddrOffsets.emplace_back(offset);
        mOffsetInObjects.emplace_back(offsetInObject);
    }

    void AppendHigh(const uint32_t offset) {
        mHighAddrOffsets.emplace_back(offset);
    }

    size_t GetNumNormal() const {
        return mNormalAddrOffsets.size();
    }

    size_t GetNumHigh() const {
        return mHighAddrOffsets.size();
    }

    uint32_t GetNormal(const size_t index) const {
        return mNormalAddrOffsets[index];
    }

    uint32_t GetHigh(const size_t index) const {
        return mHighAddrOffsets[index];
    }

    uint64_t GetOffsetInObject(const size_t index) const {
        return mOffsetInObjects[index];
    }

private:
    uint32_t mHeader = 0;

    std::vector<uint32_t> mNormalAddrOffsets;  // field offset in descriptor to store normal address

    std::vector<uint32_t> mHighAddrOffsets;    // field offset in descriptor to store high address

    std::vector<uint64_t> mOffsetInObjects;    // address offset in object

private:
    // disallow the copy constructor and assign operation
    EdmaMemoryBindings(const EdmaMemoryBindings&) = delete;
    void operator=(const EdmaMemoryBindings&) = delete;
};

#endif  // _EDMA_MEMORY_BINDINGS_H_
