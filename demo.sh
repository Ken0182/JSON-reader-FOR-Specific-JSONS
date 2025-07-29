#!/bin/bash

# Demo script for Integrated AI Synthesis System
# This script demonstrates all major features of the system

echo "=============================================="
echo "🎵 AI Synthesis System Demo 🎵"
echo "=============================================="
echo

echo "🔧 Building the system..."
make clean && make
echo

echo "🚀 Starting demonstration..."
echo

echo "📝 Demo Commands being executed:"
echo "1. Search for warm guitar sounds"
echo "2. Show system statistics"
echo "3. List all configurations"
echo "4. Select a specific configuration"
echo "5. Boost the selected configuration"
echo "6. Search again to see improved ranking"
echo "7. Find words similar to 'bright'"
echo "8. Find semantic matches for 'aggressive powerful'"
echo "9. Search with semantic expansion"
echo "10. Export current configuration state"
echo

# Create input file with demo commands
cat > demo_input.txt << EOF
search warm guitar
stats
list
select guitar_warm_acoustic
boost guitar_warm_acoustic 0.3
search warm guitar
similar bright
semantic aggressive powerful
search_semantic ambient dreamy
export demo_export.json
quit
EOF

echo "🎯 Executing demo commands..."
echo "=============================================="

# Run the demo
./ai_synthesis < demo_input.txt

echo
echo "=============================================="
echo "📊 Demo Results:"
echo "=============================================="

# Show the exported configuration
if [ -f "demo_export.json" ]; then
    echo "✅ Configuration exported successfully!"
    echo "📄 First few lines of exported config:"
    head -20 demo_export.json
    echo "..."
    echo "(Full configuration saved to demo_export.json)"
else
    echo "❌ Export failed - file not found"
fi

echo
echo "🧹 Cleaning up demo files..."
rm -f demo_input.txt

echo
echo "=============================================="
echo "🎉 Demo Complete!"
echo "=============================================="
echo
echo "🛠️  To run the system interactively:"
echo "     ./ai_synthesis"
echo
echo "📚 For more information:"
echo "     cat README_INTEGRATED.md"
echo
echo "🏗️  To rebuild:"
echo "     make clean && make"
echo