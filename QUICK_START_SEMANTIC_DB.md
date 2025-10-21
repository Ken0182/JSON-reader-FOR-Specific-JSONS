# Quick Start: Semantic Database

## 🚀 One-Line Setup

```bash
make tools && make seed-db
```

## 📊 Verify Database

```bash
./build/kbstats semantic.db
```

Expected output:
```
Dimension (D): 100
Tag Count: 148
Alias Count: 11
✓ All vectors are unit-normalized
Database Status: OK
```

## 🔍 Test Search

```bash
./build/simple_search_test
```

Expected output:
```
Query: dreamy NOT harsh
  'dreamy': rank 1 (similarity: 0.89)
  'harsh': rank 125 (similarity: 0.00)
✓ Contrastive query working as expected!
```

## 📦 What You Get

- **semantic.db** (112 KB)
  - 148 audio descriptor tags
  - 11 aliases (synthesizer→synth, etc.)
  - 100D unit-normalized embeddings
  - 7 tunable parameters
  - IDF statistics

## 🛠️ Available Tools

| Tool | Purpose | Command |
|------|---------|---------|
| seed_semantic_db | Create/update database | `./build/seed_semantic_db --db semantic.db` |
| kbstats | Check DB integrity | `./build/kbstats semantic.db` |
| test_semantic_db | Run full test suite | `./build/test_semantic_db semantic.db` |
| simple_search_test | Test contrastive search | `./build/simple_search_test` |

## ✅ Acceptance Criteria (All Met)

- [x] dimension() matches seed (100D)
- [x] All vectors unit-norm (148/148)
- [x] search "dreamy not harsh" works
- [x] kbstats prints D, tag_count, alias_count, sample tags

## 📖 Full Documentation

- `SEMANTIC_DB_DEPLOYMENT.md` - Complete deployment guide
- `SEMANTIC_DB_SUMMARY.md` - Implementation summary

## 🔄 Reseed Database (Add New Tags)

1. Edit source files (add tags to JSON or MD files)
2. Run: `make seed-db`
3. Verify: `./build/kbstats semantic.db`

No recompilation needed!

---

**Ready to use!** 🎉
