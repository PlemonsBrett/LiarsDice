#!/usr/bin/env python3
"""
Validate database schema documentation against actual implementation.
This script checks that the documented schema in database.dox matches
the actual schema defined in the code.
"""

import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple


def extract_schema_from_dox(dox_file: Path) -> Dict[str, List[Tuple[str, str]]]:
    """Extract table schemas from the documentation file."""
    schemas = {}
    current_table = None
    in_sql_block = False

    with open(dox_file, 'r') as f:
        lines = f.readlines()

    for line in lines:
        # Check for SQL code blocks
        if '@code{.sql}' in line:
            in_sql_block = True
            continue
        elif '@endcode' in line:
            in_sql_block = False
            current_table = None
            continue

        if in_sql_block:
            # Look for CREATE TABLE statements
            create_match = re.match(r'\s*CREATE TABLE (\w+)', line)
            if create_match:
                current_table = create_match.group(1)
                schemas[current_table] = []

            # Look for column definitions
            elif current_table and not line.strip().startswith('--'):
                # Match column definitions (name TYPE constraints)
                col_match = re.match(r'\s*(\w+)\s+(\w+)', line.strip())
                if col_match and col_match.group(1).upper() not in ['PRIMARY', 'FOREIGN', 'UNIQUE', 'CREATE', 'INDEX']:
                    col_name = col_match.group(1)
                    col_type = col_match.group(2)
                    schemas[current_table].append((col_name, col_type))

    return schemas


def extract_schema_from_cpp(cpp_files: List[Path]) -> Dict[str, List[Tuple[str, str]]]:
    """Extract table schemas from C++ implementation files."""
    schemas = {}

    for cpp_file in cpp_files:
        with open(cpp_file, 'r') as f:
            content = f.read()

        # Look for CREATE TABLE statements in strings
        create_tables = re.findall(
            r'CREATE TABLE\s+(?:IF NOT EXISTS\s+)?(\w+)\s*\((.*?)\)',
            content,
            re.DOTALL | re.IGNORECASE
        )

        for table_name, table_def in create_tables:
            schemas[table_name] = []
            # Parse column definitions
            lines = table_def.split(',')
            for line in lines:
                line = line.strip()
                if line and not any(kw in line.upper() for kw in ['PRIMARY KEY', 'FOREIGN KEY', 'UNIQUE', 'INDEX']):
                    col_match = re.match(r'(\w+)\s+(\w+)', line)
                    if col_match:
                        col_name = col_match.group(1)
                        col_type = col_match.group(2)
                        schemas[table_name].append((col_name, col_type))

    return schemas


def compare_schemas(doc_schemas: Dict, impl_schemas: Dict) -> List[str]:
    """Compare documented schemas with implementation schemas."""
    issues = []

    # Check for tables in docs but not in implementation
    doc_tables = set(doc_schemas.keys())
    impl_tables = set(impl_schemas.keys())

    missing_in_impl = doc_tables - impl_tables
    if missing_in_impl:
        for table in missing_in_impl:
            issues.append(f"Table '{table}' documented but not implemented")

    missing_in_docs = impl_tables - doc_tables
    if missing_in_docs:
        for table in missing_in_docs:
            issues.append(f"Table '{table}' implemented but not documented")

    # Check column consistency for common tables
    common_tables = doc_tables & impl_tables
    for table in common_tables:
        doc_cols = {col[0]: col[1] for col in doc_schemas[table]}
        impl_cols = {col[0]: col[1] for col in impl_schemas[table]}

        # Check for missing columns
        missing_cols = set(doc_cols.keys()) - set(impl_cols.keys())
        if missing_cols:
            for col in missing_cols:
                issues.append(f"Column '{table}.{col}' documented but not implemented")

        extra_cols = set(impl_cols.keys()) - set(doc_cols.keys())
        if extra_cols:
            for col in extra_cols:
                issues.append(f"Column '{table}.{col}' implemented but not documented")

        # Check type mismatches
        for col in set(doc_cols.keys()) & set(impl_cols.keys()):
            if doc_cols[col].upper() != impl_cols[col].upper():
                issues.append(
                    f"Type mismatch for '{table}.{col}': documented as {doc_cols[col]}, implemented as {impl_cols[col]}")

    return issues


def main():
    """Main validation function."""
    project_root = Path(__file__).parent.parent

    # Find documentation file
    dox_file = project_root / "documentation" / "pages" / "database.dox"
    if not dox_file.exists():
        print(f"Error: Documentation file not found: {dox_file}")
        sys.exit(1)

    # Find C++ implementation files
    cpp_files = []
    include_dir = project_root / "include" / "liarsdice" / "database"
    source_dir = project_root / "source" / "liarsdice" / "database"

    if include_dir.exists():
        cpp_files.extend(include_dir.glob("*.hpp"))
    if source_dir.exists():
        cpp_files.extend(source_dir.glob("*.cpp"))

    if not cpp_files:
        print("Warning: No database implementation files found")
        print("Skipping schema validation - implementation may not be complete yet")
        sys.exit(0)

    # Extract schemas
    print("Extracting schema from documentation...")
    doc_schemas = extract_schema_from_dox(dox_file)
    print(f"Found {len(doc_schemas)} tables in documentation")

    print("Extracting schema from implementation...")
    impl_schemas = extract_schema_from_cpp(cpp_files)
    print(f"Found {len(impl_schemas)} tables in implementation")

    # Compare schemas
    print("\nValidating schema consistency...")
    issues = compare_schemas(doc_schemas, impl_schemas)

    if issues:
        print("\n⚠️  Schema validation issues found:")
        for issue in issues:
            print(f"  - {issue}")
        sys.exit(1)
    else:
        print("✅ Schema documentation and implementation are consistent")
        sys.exit(0)


if __name__ == "__main__":
    main()
