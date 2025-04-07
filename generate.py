#!/usr/bin/env python3
import random
import argparse

def generate_fenwick_tree_input(size, num_operations, output_file, query_percentage=20):
    """
    Generate a test input file for the Fenwick Tree implementation.
    
    Args:
        size: Size of the Fenwick tree
        num_operations: Number of operations to generate
        output_file: Path to the output file
        query_percentage: Percentage of operations that should be queries (default: 20%)
    """
    with open(output_file, 'w') as f:
        # Write the size of the Fenwick tree
        f.write(f"{size} {num_operations}\n")
        
        # Generate random operations
        for _ in range(num_operations):
            # Decide operation type with probabilities
            op_type = random.choices(['a', 'q'], 
                                    weights=[(100 - query_percentage), 
                                             query_percentage], 
                                    k=1)[0]
            
            # Generate random index and value
            index = random.randint(0, size - 1)
            
            if op_type == 'q':
                # Query operation
                f.write(f"q {index}\n")
            else:
                # Add or delete operation
                value = random.randint(1, 100)
                f.write(f"{op_type} {index} {value}\n")

def main():
    parser = argparse.ArgumentParser(description='Generate test input for Fenwick Tree')
    parser.add_argument('--size', type=int, default=128, help='Size of the Fenwick tree')
    parser.add_argument('--operations', type=int, default=1000, help='Number of operations to generate')
    parser.add_argument('--output', type=str, default='input.txt', help='Output file path')
    parser.add_argument('--queries', type=int, default=20, help='Percentage of query operations (0-100)')
    
    args = parser.parse_args()
    
    # Validate input
    if args.size <= 0:
        parser.error("Size must be positive")
    if args.operations <= 0:
        parser.error("Number of operations must be positive")
    if not 0 <= args.queries <= 100:
        parser.error("Query percentage must be between 0 and 100")
    
    generate_fenwick_tree_input(args.size, args.operations, args.output, args.queries)
    print(f"Generated {args.operations} operations for a Fenwick tree of size {args.size}")
    print(f"Output written to {args.output}")

if __name__ == "__main__":
    main()