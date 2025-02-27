/*
 * Copyright 2022 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/graphite/PipelineData.h"

#include "src/core/SkOpts.h"
#include "src/gpu/graphite/ShaderCodeDictionary.h"

namespace skgpu::graphite {

using SnippetRequirementFlags = SnippetRequirementFlags;

PipelineDataGatherer::PipelineDataGatherer(skgpu::graphite::Layout layout)
        : fUniformManager(layout)
        , fSnippetRequirementFlags(SnippetRequirementFlags::kNone) {
}

void PipelineDataGatherer::reset() {
    fTextureDataBlock.reset();
    fUniformManager.reset();
    fSnippetRequirementFlags = SnippetRequirementFlags::kNone;
}

#ifdef SK_DEBUG
void PipelineDataGatherer::checkReset() {
    SkASSERT(fTextureDataBlock.empty());
    SkDEBUGCODE(fUniformManager.checkReset());
    SkASSERT(fSnippetRequirementFlags == SnippetRequirementFlags::kNone);
}
#endif // SK_DEBUG

void PipelineDataGatherer::addFlags(SkEnumBitMask<SnippetRequirementFlags> flags) {
    fSnippetRequirementFlags |= flags;
}

bool PipelineDataGatherer::needsLocalCoords() const {
    return fSnippetRequirementFlags & SnippetRequirementFlags::kLocalCoords;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UniformDataBlock* UniformDataBlock::Make(const UniformDataBlock& other, SkArenaAlloc* arena) {
    static constexpr size_t kUniformAlignment = alignof(void*);
    char* mem = static_cast<char*>(arena->makeBytesAlignedTo(other.size(), kUniformAlignment));
    memcpy(mem, other.data(), other.size());

    return arena->make([&](void* ptr) {
        return new (ptr) UniformDataBlock(SkSpan<const char>(mem, other.size()));
    });
}

uint32_t UniformDataBlock::hash() const {
    return SkOpts::hash_fn(fData.data(), fData.size(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TextureDataBlock* TextureDataBlock::Make(const TextureDataBlock& other,
                                             SkArenaAlloc* arena) {
    return arena->make([&](void *ptr) {
        return new (ptr) TextureDataBlock(other);
    });
}

bool TextureDataBlock::operator==(const TextureDataBlock& other) const {
    if (fTextureData.size() != other.fTextureData.size()) {
        return false;
    }

    for (size_t i = 0; i < fTextureData.size(); ++i) {
        if (fTextureData[i] != other.fTextureData[i]) {
            return false;
        }
    }

    return true;
}

uint32_t TextureDataBlock::hash() const {
    uint32_t hash = 0;

    for (auto& d : fTextureData) {
        uint32_t samplerKey = std::get<1>(d).asKey();
        hash = SkOpts::hash_fn(&samplerKey, sizeof(samplerKey), hash);

        // Because the lifetime of the TextureDataCache is for just one Recording and the
        // TextureDataBlocks hold refs on their proxies, we can just use the proxy's pointer
        // for the hash here.
        uintptr_t proxy = reinterpret_cast<uintptr_t>(std::get<0>(d).get());
        hash = SkOpts::hash_fn(&proxy, sizeof(proxy), hash);
    }

    return hash;
}

} // namespace skgpu::graphite
