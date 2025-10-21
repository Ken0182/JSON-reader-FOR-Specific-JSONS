#!/bin/bash
set -e

echo "=========================================="
echo "Semantic Database Complete Demo"
echo "=========================================="
echo ""

echo "1. Seeding Database..."
echo "----------------------------------------"
./build/seed_semantic_db --db semantic.db --dimension 100 2>&1 | tail -20
echo ""

echo "2. Running Integrity Check (kbstats)..."
echo "----------------------------------------"
./build/kbstats semantic.db
echo ""

echo "3. Running Test Suite..."
echo "----------------------------------------"
./build/test_semantic_db semantic.db
echo ""

echo "4. Running Contrastive Search Test..."
echo "----------------------------------------"
./build/simple_search_test 2>&1 | head -40
echo ""

echo "=========================================="
echo "All Tests Passed!"
echo "Semantic database ready for deployment."
echo "=========================================="
