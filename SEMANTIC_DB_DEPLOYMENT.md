# Semantic Database Deployment Guide

## Overview

This document describes the semantic knowledge base database (`semantic.db`) that replaces hardcoded embeddings with a flexible SQLite-based system.

## What it Does

The semantic.db system:
- **Stores tags, aliases, and embeddings** - No more hardcoded vocabulary
- **Supports unlimited vocabulary** - Add new tags without recompiling
- **Maintains tunable parameters** - Adjust semantic weights in the database
- **Provides IDF statistics** - Weighted by tag informativeness
- **Ensures unit-norm vectors** - All embeddings are L2-normalized

## Database Schema

```sql
-- Tags table: canonical forms and dimensions
CREATE TABLE tags (
    tag TEXT PRIMARY KEY,
    canonical TEXT NOT NULL,
    dimension INTEGER NOT NULL DEFAULT 100
);

-- Embeddings table: vector storage
CREATE TABLE embeddings (
    tag TEXT PRIMARY KEY,
    embedding BLOB NOT NULL,        -- Binary float array
    dimension INTEGER NOT NULL,
    FOREIGN KEY (tag) REFERENCES tags(tag)
);

-- IDF statistics: tag informativeness
CREATE TABLE idf_stats (
    tag TEXT PRIMARY KEY,
    idf REAL NOT NULL,
    doc_count INTEGER NOT NULL DEFAULT 0
);

-- Configuration: tunable parameters
CREATE TABLE config (
    key TEXT PRIMARY KEY,
    value REAL NOT NULL
);
```

## Tools

### 1. seed_semantic_db

Seeds the semantic database from repository data sources.

**Usage:**
```bash
./build/seed_semantic_db --db semantic.db --dimension 100
```

**Data Sources:**
- All `*.md` files in the repository
- `clean_config.json`, `group.json`, `guitar.json`, `structure.json`
- Common audio descriptor patterns (regex-based extraction)

**Output:**
- Creates/replaces `semantic.db`
- Extracts 100+ unique tags
- Generates embeddings using hash-based encoder
- Stores aliases (e.g., "synthesizer" → "synth")
- Populates tunable parameters

### 2. kbstats

Prints database integrity statistics.

**Usage:**
```bash
./build/kbstats semantic.db
```

**Output:**
```
Knowledge Base Statistics
==========================================
Database: semantic.db

Dimension (D): 100
Tag Count: 148
Alias Count: 11

Sample Tags (first 10):
  - acid |v|=1.000
  - acoustic |v|=1.000
  - aggressive |v|=1.000
  ...

Vector Norm Check:
  Unit-normalized vectors: 148/148
  ✓ All vectors are unit-normalized

==========================================
Database Status: OK
```

### 3. test_semantic_db

Comprehensive integrity test suite.

**Usage:**
```bash
./build/test_semantic_db semantic.db
```

**Tests:**
1. Dimension check (matches seed)
2. Unit norm verification (all vectors |v|=1.0)
3. Database statistics (tag count, sample tags)
4. Alias resolution (synonyms → canonical forms)
5. Semantic similarity (cosine distance)

### 4. simple_search_test

Tests contrastive query functionality.

**Usage:**
```bash
./build/simple_search_test
```

**Example Query:** "dreamy NOT harsh"

**Output:**
```
Top 15 results (query: dreamy NOT harsh):
Rank  Tag                Similarity  Why
----  -----------------  ----------  ---
   1  dreamy                 0.8918  Exact match to positive term
   2  comp                   0.2228  Moderate similarity
...
 125  harsh                  0.0000  Negative term (should rank lower)

Verification:
  ✓ PASS: 'dreamy' ranks higher than 'harsh'
  ✓ Contrastive query working as expected!
```

## Makefile Targets

```bash
# Build tools
make tools                  # Build all semantic DB tools

# Seed database
make seed-db                # Run seed_semantic_db

# Clean
make clean                  # Remove build artifacts and semantic.db
```

## Integration with Main System

The semantic.db is loaded by `SemanticKnowledgeBase`:

```cpp
// Initialize knowledge base
SemanticKnowledgeBase kb("semantic.db");
kb.initialize(false);  // Don't create default embeddings

// Get tag embedding
auto warmVec = kb.getTagEmbedding("warm");

// Encode arbitrary text
auto queryVec = kb.encodeText("dreamy but not harsh");

// Compute similarity
float sim = SemanticKnowledgeBase::cosineSimilarity(warmVec, queryVec);
```

## Acceptance Criteria

All criteria met:

✅ **dimension() matches seed**
- Database reports D=100
- All embeddings are 100-dimensional

✅ **All vectors are unit-norm**
- 148/148 vectors have |v|=1.000 (within 0.01 tolerance)
- Verified by kbstats and test_semantic_db

✅ **search "dreamy not harsh --why" returns sensible top-N**
- "dreamy" ranks #1 (similarity: 0.89)
- "harsh" ranks #125 (similarity: 0.00)
- Contrastive query working as expected

✅ **kbstats prints D, tag_count, alias_count, sample tags**
- All statistics displayed correctly
- Database integrity verified

## Benefits

1. **No Hardcoded Vocabulary** - Add tags by reseeding database
2. **Fast Deployment** - Update embeddings without recompilation
3. **Persistent Storage** - SQLite for reliable data management
4. **Tunable Parameters** - Adjust weights in config table
5. **Unlimited Expansion** - Add thousands of tags without code changes

## Future Enhancements

1. **Pre-trained Embeddings** - Load Word2Vec/GloVe/FastText models
2. **Contextual Embeddings** - BERT-style sentence encoders
3. **Multi-lingual Support** - Tags in multiple languages
4. **Acoustic Features** - Combine text with audio spectra
5. **Dynamic IDF Updates** - Recompute from usage statistics

## Files Created

```
tools/
├── seed_semantic_db.cpp      # Database seeder
├── kbstats.cpp                # Statistics tool
├── test_semantic_db.cpp       # Integrity test
└── simple_search_test.cpp     # Search test

semantic.db                     # SQLite database (114KB)
```

## Verification Commands

```bash
# Build everything
make clean && make && make tools

# Seed database
make seed-db

# Verify integrity
./build/kbstats semantic.db
./build/test_semantic_db semantic.db

# Test search
./build/simple_search_test
```

All tests pass ✓

---

**Version:** 1.0  
**Created:** 2025-10-21  
**Database Schema:** v1.6
