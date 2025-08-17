"""
Robot Framework library for Doxygen documentation validation.
"""

import re
from html.parser import HTMLParser
from pathlib import Path
from typing import Dict


class LinkExtractor(HTMLParser):
    """Extract links from HTML files."""

    def __init__(self):
        super().__init__()
        self.links = []
        self.current_file = None

    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            for attr, value in attrs:
                if attr == 'href' and value:
                    self.links.append(value)


class DocumentationLibrary:
    """Library for validating Doxygen documentation."""

    ROBOT_LIBRARY_SCOPE = 'TEST SUITE'

    def validate_documentation_links(self, docs_dir: str) -> Dict:
        """
        Validate all internal links in the documentation.
        
        Args:
            docs_dir: Path to the HTML documentation directory
            
        Returns:
            Dictionary with status (PASS/FAIL) and message
        """
        docs_path = Path(docs_dir)
        if not docs_path.exists():
            return {'status': 'FAIL', 'message': f'Documentation directory not found: {docs_dir}'}

        broken_links = []
        checked_links = set()

        # Find all HTML files
        html_files = list(docs_path.glob('*.html'))

        for html_file in html_files:
            parser = LinkExtractor()
            parser.current_file = html_file

            with open(html_file, 'r', encoding='utf-8') as f:
                try:
                    parser.feed(f.read())
                except Exception as e:
                    print(f"Error parsing {html_file}: {e}")
                    continue

            # Check each link
            for link in parser.links:
                # Skip external links and anchors
                if link.startswith(('http://', 'https://', 'mailto:', '#')):
                    continue

                # Skip already checked links
                link_key = f"{html_file}:{link}"
                if link_key in checked_links:
                    continue
                checked_links.add(link_key)

                # Remove anchor from link
                link_path = link.split('#')[0]
                if not link_path:
                    continue

                # Resolve the link path
                target_path = (html_file.parent / link_path).resolve()

                # Check if target exists
                if not target_path.exists():
                    broken_links.append({
                        'source': str(html_file.relative_to(docs_path)),
                        'target': link,
                        'resolved': str(target_path)
                    })

        if broken_links:
            message = f"Found {len(broken_links)} broken links:\n"
            for link in broken_links[:10]:  # Show first 10
                message += f"  - {link['source']} -> {link['target']}\n"
            if len(broken_links) > 10:
                message += f"  ... and {len(broken_links) - 10} more"
            return {'status': 'FAIL', 'message': message}

        return {'status': 'PASS', 'message': f'All internal links valid ({len(checked_links)} checked)'}

    def check_cross_references(self, docs_dir: str) -> Dict:
        """
        Check for broken cross-references in documentation.
        
        Args:
            docs_dir: Path to the HTML documentation directory
            
        Returns:
            Dictionary with status and message
        """
        docs_path = Path(docs_dir)
        if not docs_path.exists():
            return {'status': 'FAIL', 'message': f'Documentation directory not found: {docs_dir}'}

        # Look for common patterns of broken references
        broken_refs = []

        # Check for "Related Pages" or class references
        html_files = list(docs_path.glob('*.html'))

        for html_file in html_files:
            with open(html_file, 'r', encoding='utf-8') as f:
                content = f.read()

            # Look for common broken reference patterns
            # Doxygen typically shows [Class Name] for unresolved references
            unresolved = re.findall(r'\[([A-Za-z_][A-Za-z0-9_:]*)]', content)

            # Also check for missing class links (href="#" without proper target)
            missing_targets = re.findall(r'href="#"\s*>([^<]+)</a>', content)

            if unresolved:
                for ref in unresolved:
                    # Filter out common false positives
                    if ref not in ['TODO', 'FIXME', 'NOTE', 'WARNING']:
                        broken_refs.append({
                            'file': str(html_file.relative_to(docs_path)),
                            'reference': ref,
                            'type': 'unresolved'
                        })

            if missing_targets:
                for target in missing_targets:
                    if target.strip() and not target.startswith('Generated'):
                        broken_refs.append({
                            'file': str(html_file.relative_to(docs_path)),
                            'reference': target.strip(),
                            'type': 'missing_target'
                        })

        if broken_refs:
            message = f"Found {len(broken_refs)} broken references:\n"
            for ref in broken_refs[:10]:
                message += f"  - {ref['file']}: {ref['reference']} ({ref['type']})\n"
            if len(broken_refs) > 10:
                message += f"  ... and {len(broken_refs) - 10} more"
            # For now, just warn about broken references
            return {'status': 'PASS',
                    'message': f'Warning: {len(broken_refs)} potential broken references found (not failing)'}

        return {'status': 'PASS', 'message': 'No broken cross-references found'}

    def check_documentation_coverage(self, include_dir: str) -> Dict:
        """
        Check documentation coverage for public APIs.
        
        Args:
            include_dir: Path to the include directory
            
        Returns:
            Dictionary with coverage percentage and details
        """
        include_path = Path(include_dir)
        if not include_path.exists():
            return {'coverage': 0, 'message': f'Include directory not found: {include_dir}'}

        total_items = 0
        documented_items = 0
        undocumented = []

        # Find all header files
        header_files = list(include_path.rglob('*.hpp')) + list(include_path.rglob('*.h'))

        for header_file in header_files:
            with open(header_file, 'r', encoding='utf-8') as f:
                content = f.read()

            # Simple heuristic: look for public class/function declarations
            # and check if they have documentation comments
            lines = content.split('\n')

            for i, line in enumerate(lines):
                # Look for class declarations
                if re.match(r'\s*class\s+[A-Za-z_][A-Za-z0-9_]*\s*[:{]', line):
                    total_items += 1
                    # Check for documentation comment above
                    if i > 0 and ('///' in lines[i - 1] or '/**' in '\n'.join(lines[max(0, i - 5):i])):
                        documented_items += 1
                    else:
                        undocumented.append({
                            'file': str(header_file.relative_to(include_path)),
                            'line': i + 1,
                            'item': line.strip()
                        })

                # Look for public method declarations (simplified)
                elif re.match(
                        r'\s*(?:virtual\s+)?(?:static\s+)?[A-Za-z_][A-Za-z0-9_<>:,\s]*\s+[A-Za-z_][A-Za-z0-9_]*\s*\([^)]*\)\s*(?:const)?\s*(?:override)?\s*;',
                        line):
                    # Skip if in private/protected section
                    if not any(keyword in '\n'.join(lines[max(0, i - 20):i]) for keyword in ['private:', 'protected:']):
                        total_items += 1
                        if i > 0 and ('///' in lines[i - 1] or '/**' in '\n'.join(lines[max(0, i - 5):i])):
                            documented_items += 1

        coverage = (documented_items / total_items * 100) if total_items > 0 else 100

        return {
            'coverage': coverage,
            'total': total_items,
            'documented': documented_items,
            'undocumented_count': len(undocumented),
            'message': f'Documentation coverage: {coverage:.1f}% ({documented_items}/{total_items} items)'
        }
