#!/usr/bin/env python3
##
## license:BSD-3-Clause
## copyright-holders:stonedDiscord

import json, os, re, sys, subprocess
from pathlib import Path

def is_screaming_snake(name: str):
	return re.fullmatch(r"[A-Z][A-Z0-9_]*", name) is not None

def is_snake_case(name: str):
	return re.fullmatch(r"[a-z][a-z0-9_]*(_[a-z0-9]+)*", name) is not None

# C++ keywords that look like function calls (keyword followed by '(') but are not function names.
CPP_CONTROL_KEYWORDS = {
	"if", "for", "while", "switch", "return", "sizeof", "alignof", "decltype",
	"do", "catch", "throw", "case", "delete", "new", "and", "or", "not",
	"static_cast", "dynamic_cast", "reinterpret_cast", "const_cast",
}

def strip_literals(line: str):
	"""Replace the contents of string/char literals with spaces so that checks
	for hex literals and block comments don't trigger on text inside strings."""
	return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
		lambda m: '"' + ' ' * (len(m.group(0)) - 2) + '"' if m.group(0)[0] == '"'
		else "'" + ' ' * (len(m.group(0)) - 2) + "'", line)

def read_source(path: Path):
	"""Read a file as bytes and strict UTF-8 text. Returns (raw_bytes, text, error)."""
	try:
		raw_bytes = path.read_bytes()
	except Exception:
		return None, None, None
	try:
		text = raw_bytes.decode("utf-8", errors="strict")
	except UnicodeDecodeError as e:
		return raw_bytes, None, (1, f"Invalid UTF-8 sequence at byte {e.start}-{e.end}")
	return raw_bytes, text, None

def check_file(path: Path, fix: bool = False, raw_bytes=None, text=None):
	errors = []
	if raw_bytes is None or text is None:
		raw_bytes, text, error = read_source(path)
		if error is not None:
			return [error]
		if raw_bytes is None or text is None:
			return errors

	has_crlf = b"\r\n" in raw_bytes
	if sys.platform != "win32" and has_crlf:
		errors.append((1, "File contains Windows (CRLF) line endings on a non-Windows system"))
	elif sys.platform == "win32" and b"\n" in raw_bytes and not has_crlf:
		errors.append((1, "File contains Unix (LF) line endings on Windows"))

	lines = text.splitlines()

	needs_newline = not text.endswith("\n")
	if needs_newline:
		errors.append((len(lines) or 1, "File should end with a newline"))

	has_trailing_ws = False
	for i, line in enumerate(lines, 1):
		if line.rstrip() != line and line.strip():
			has_trailing_ws = True
			errors.append((i, "Lines should not have trailing whitespaces, consider running srcclean"))

	if fix and (needs_newline or has_trailing_ws or (sys.platform != "win32" and has_crlf)):
		fixed = "\n".join(l.rstrip() if l.strip() else l for l in lines)
		path.write_text(fixed + "\n", encoding="utf-8", newline="\n")

	return errors

def check_spdx_header(lines):
	license_pattern = re.compile(r'^//\s*license:')
	copyright_pattern = re.compile(r'^//\s*copyright-holders:')

	errors = []
	i = 0

	while i < len(lines) and not lines[i].strip():
		i += 1

	if i >= len(lines):
		errors.append((i + 1, "Missing // license: header"))
	elif not license_pattern.match(lines[i]) or not lines[i].split(':', 1)[1].strip():
		errors.append((i + 1, "Incorrect or empty // license: header"))

	i += 1
	while i < len(lines) and not lines[i].strip():
		i += 1

	if i >= len(lines):
		errors.append((i + 1, "Missing // copyright-holders: header"))
	elif not copyright_pattern.match(lines[i]) or not lines[i].split(':', 1)[1].strip():
		errors.append((i + 1, "Incorrect or empty // copyright-holders: header"))

	return errors

def check_includes(path: Path, lines):
	errors = []
	includes = []
	for i, line in enumerate(lines):
		stripped = line.strip()
		if stripped.startswith('#include'):
			includes.append((i + 1, stripped))

	if not includes:
		return errors

	def get_group(include_line):
		m = re.match(r'#include\s+["<]([^">]+)[">]', include_line)
		if not m:
			return None
		inc_path = m.group(1)
		if inc_path == 'emu.h':
			return 0
		if inc_path == 'logmacro.h':
			return 5
		if include_line.endswith('>'):
			return 3
		else:
			if '/' in inc_path:
				return 1
			else:
				if inc_path.endswith('.lh'):
					return 4
				else:
					return 2

	include_data = []
	for line_no, inc_line in includes:
		group = get_group(inc_line)
		if group is None:
			continue
		m = re.match(r'#include\s+["<]([^">]+)[">]', inc_line)
		inc_path = m.group(1)
		include_data.append((group, inc_path, line_no))

	expected = sorted(include_data, key=lambda x: (x[0], x[1].lower()))
	for (exp_group, exp_path, _), (act_group, act_path, act_line) in zip(expected, include_data):
		if exp_group != act_group or exp_path != act_path:
			errors.append((act_line, f"Include '{act_path}' is out of order, expected '{exp_path}'"))
			break

	for i in range(len(includes) - 1):
		line_no1, inc_line1 = includes[i]
		line_no2, inc_line2 = includes[i + 1]
		group1 = get_group(inc_line1)
		group2 = get_group(inc_line2)
		if group1 != group2:
			has_blank = any(lines[j].strip() == '' for j in range(line_no1, line_no2))
			if not has_blank:
				errors.append((line_no2, "Missing blank line between include groups"))

	return errors

def check_rom_regions(lines):
	errors = []
	rom_start_pattern = re.compile(r'^\s*ROM_START\s*\(')
	rom_end_pattern = re.compile(r'^\s*ROM_END\s*$')
	rom_region_pattern = re.compile(r'^\s*ROM_REGION')

	inside_rom = False
	first_region = True

	for i, line in enumerate(lines, 1):
		if rom_start_pattern.match(line):
			inside_rom = True
			first_region = True
		elif rom_end_pattern.match(line):
			inside_rom = False
			if i < len(lines) and lines[i].strip() != '':
				errors.append((i + 1, "ROM_END should be followed by a blank line"))
		elif inside_rom and rom_region_pattern.match(line):
			if first_region:
				if i >= 2 and lines[i - 2].strip() == '':
					errors.append((i, "First ROM_REGION should not have a blank line before it"))
				first_region = False
			else:
				if i <= 1 or lines[i - 2].strip() != '':
					errors.append((i, "ROM_REGION should have a blank line before it"))

	return errors

def check_cpp_file(path: Path, fix: bool = False):
	raw_bytes, text, error = read_source(path)
	if error is not None:
		return [error]
	if text is None:
		return []

	errors = check_file(path, fix, raw_bytes=raw_bytes, text=text)

	lines = text.splitlines()

	errors.extend(check_spdx_header(lines))

	errors.extend(check_includes(path, lines))

	for i, line in enumerate(lines, 1):
		# Strip string/char literals so checks don't trigger on their contents.
		code = strip_literals(line)

		if re.search(r"\b0x[0-9a-fA-F]*[A-F][0-9a-fA-F]*\b", code):
			errors.append((i, "Hex literals should be lowercase (0x1a not 0x1A)"))

		if re.search(r"/\*.*\*/", code.strip()):
			errors.append((i, "/* Single-line block comments */ should use // instead"))

		m = re.match(r"^\s*#define\s+([A-Za-z0-9_]+)", line)
		if m and not is_screaming_snake(m.group(1)):
			errors.append((i, f"Macro '{m.group(1)}' should use SCREAMING_SNAKE_CASE"))

		if re.match(r"^\s*#define\s+VERBOSE\s+1\b", line):
			errors.append((i, "VERBOSE not set to 0 or commented out"))

		# Function names: only inspect plausible definitions/declarations, not
		# every call. Skip control keywords and member/scoped invocations.
		for f in re.finditer(r"(?<![.\w>])\b([a-z][A-Za-z0-9_]*)\s*\(", code):
			# Skip scoped/member calls like ns::func( or obj->func(.
			start = f.start(1)
			if start >= 2 and code[start - 2:start] in ("::",):
				continue
			name = f.group(1)
			if name in CPP_CONTROL_KEYWORDS:
				continue
			if not is_snake_case(name):
				errors.append((i, f"Function '{name}' should use snake_case"))

		# enum (class) name: handle before plain class so 'enum class X' is not
		# double-reported, and skip 'enum class' for the class check below.
		en = re.search(r"\benum\s+(?:class\s+|struct\s+)?([A-Za-z0-9_]+)", line)
		if en and not is_snake_case(en.group(1)):
			errors.append((i, f"Enum '{en.group(1)}' should use snake_case"))

		cl = re.search(r"(?<!enum )\bclass\s+([A-Za-z0-9_]+)", line)
		if cl and not is_snake_case(cl.group(1)):
			errors.append((i, f"Class '{cl.group(1)}' should use snake_case"))

	errors.extend(check_rom_regions(lines))

	return errors

def natural_sort(l):
	convert = lambda text: int(text) if text.isdigit() else text.lower()
	alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
	return sorted(l, key=alphanum_key)

def check_lst_block(block, changed_cpp_files, start_line, src_file):
	if not src_file or src_file not in changed_cpp_files:
		return []
	sorted_block = natural_sort(block)
	errors_local = []
	for offset, (expected, actual) in enumerate(zip(sorted_block, block)):
		if expected != actual:
			line_no = start_line + offset + 1
			errors_local.append(
				(line_no, f"Entry '{actual}' is out of order, expected '{expected}'")
			)
	return errors_local

def check_mame_lst(changed_cpp_files: set[str]):
	path = Path("src/mame/mame.lst")
	errors = check_file(path, False)

	try:
		lines = path.read_text().splitlines()
	except Exception:
		return errors

	current_block = []
	block_start_line = 0
	current_source = None

	for i, line in enumerate(lines):
		if line.startswith("@source:"):
			if current_block:
				errors.extend(check_lst_block(current_block, changed_cpp_files, block_start_line, current_source))
			current_source = "src/mame/" + line[len("@source:"):].strip()
			current_block = []
			block_start_line = i + 1
		elif line.strip():
			current_block.append(line.strip())

	if current_block:
		errors.extend(check_lst_block(current_block, changed_cpp_files, block_start_line, current_source))

	return errors

def parse_game_entries(content):
	pattern = r'(GAME|GAMEL|CONS|SYST)\(\s*\d+,\s*([a-zA-Z0-9_]+),\s*([^,]+)'
	games = []
	
	for match in re.finditer(pattern, content):
		name = match.group(2)
		parent = match.group(3).strip()
		is_clone = parent != '0'
		games.append({'name': name, 'parent': parent, 'is_clone': is_clone})
	
	return games

def find_game_line_number(content, game_name):
	pattern = r'(GAME|GAMEL|CONS|SYST)\(\s*\d+,\s*' + re.escape(game_name) + r'\s*,'
	
	lines = content.splitlines()
	for i, line in enumerate(lines, 1):
		if re.search(pattern, line):
			return i
	
	return 1  # Return 1 instead of 0 for better error reporting

def parse_mame_lst():
	path = Path("src/mame/mame.lst")
	games = set()
	try:
		lines = path.read_text().splitlines()
		for line in lines:
			line = line.strip()
			if line and not line.startswith("@"):
				games.add(line)
	except Exception:
		pass
	return games

def check_game_entries_vs_lst(cpp_files):
	errors = []
	mame_games = parse_mame_lst()
	cpp_games = set()

	for cpp_file in cpp_files:
		path = Path(cpp_file)
		try:
			content = path.read_text()
			games = parse_game_entries(content)
			for game in games:
				cpp_games.add(game['name'])
				if not game['is_clone'] and len(game['name']) > 8:
					line_no = find_game_line_number(content, game['name'])
					errors.append((line_no, f"Game '{game['name']}' has no parent/clone but name is longer than 8 characters"))
		except Exception as e:
			errors.append((1, f"Error parsing {cpp_file}: {e}"))

	missing = cpp_games - mame_games
	for game in missing:
		errors.append((1, f"Game '{game}' found in cpp files but missing from mame.lst"))

	return errors

def print_review(path, lineno, msg, out=None):
	review = {"body": str(msg), "path": str(path), "line": int(lineno)}
	if out:
		out.write(json.dumps(review)+'\n')
	else:
		print(json.dumps(review))

def _escape_annotation_data(s):
	return str(s).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")

def _escape_annotation_property(s):
	return _escape_annotation_data(s).replace(":", "%3A").replace(",", "%2C")

def print_annotations(comments):
	"""Emit GitHub Actions annotation commands so issues show up inline on the
	pull request diff. Needs no token, so it works on pull requests from forks."""
	for comment in comments:
		path = _escape_annotation_property(comment["path"])
		line = int(comment["line"])
		title = _escape_annotation_property("style.py")
		body = _escape_annotation_data(comment["body"])
		print(f"::error file={path},line={line},title={title}::{body}")

def execute_git_command(cmds, timeout=30, description="Git command"):
	"""Execute git commands with fallback and error handling."""
	for cmd in cmds:
		try:
			result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
			if result.returncode == 0:
				return result
		except subprocess.TimeoutExpired:
			raise RuntimeError(f"{description} timed out ({timeout}s)")
		except Exception as e:
			raise RuntimeError(f"Error executing {description}: {e}") from e
	
	raise RuntimeError(f"{description} failed: {result.stderr if 'result' in locals() else 'Unknown error'}")

def get_changed_lines(file_path, base_branch="master", head_branch="HEAD"):
	"""Get changed lines in a file between two branches."""
	cmds = [
		['git', 'diff', '--unified=0', f'{base_branch}...{head_branch}', '--', file_path],
		['git', 'diff', '--unified=0', f'{base_branch}..{head_branch}', '--', file_path]
	]
	
	result = execute_git_command(cmds, timeout=30, description=f"Git diff for {file_path}")
	
	changed_lines = set()
	for line in result.stdout.splitlines():
		if line.startswith('@@'):
			m = re.match(r'@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@', line)
			if m:
				new_start = int(m.group(1))
				new_count = int(m.group(2) or 1)
				changed_lines.update(range(new_start, new_start + new_count))
	
	return changed_lines

def get_changed_files(base_branch="master", head_branch="HEAD"):
	"""Get changed files between two branches."""
	cmds = [
		['git', 'diff', '--name-only', '--diff-filter=ACMRT', f'{base_branch}...{head_branch}'],
		['git', 'diff', '--name-only', '--diff-filter=ACMRT', f'{base_branch}..{head_branch}']
	]
	
	result = execute_git_command(cmds, timeout=30, description="Git diff for changed files")
	stripped = result.stdout.strip()
	return set(stripped.split('\n')) if stripped else set()

def categorize_files(file_list):
	"""Categorize files by their extensions."""
	cpp_files = {f for f in file_list if f.endswith((".c", ".cpp"))}
	h_files = {f for f in file_list if f.endswith((".h", ".hpp", ".hxx", ".ipp"))}
	other_files = {f for f in file_list if f.endswith((".py", ".lua", ".mm", ".lay", ".lst"))}
	return cpp_files, h_files, other_files

def create_file_comments(errors, path):
	"""Create comment objects from error tuples."""
	comments = []
	for lineno, msg in errors:
		comments.append({
			"body": str(msg),
			"path": str(path),
			"line": int(lineno)
		})
	return comments

def filter_comments_for_ci(file_comments, base_branch, head_branch):
	"""Filter comments for CI mode to only show errors in changed lines."""
	changed_files = get_changed_files(base_branch, head_branch)
	filtered_comments = []

	changed_lines_cache = {}

	for file in file_comments:
		file_changed = file in changed_files
		file_path = file

		if file == "src/mame/mame.lst":
			try:
				mame_lst_path = Path("src/mame/mame.lst")
				lines = mame_lst_path.read_text().splitlines()
				source_files = set()
				for line in lines:
					if line.startswith("@source:"):
						source_file = "src/mame/" + line[len("@source:"):].strip()
						source_files.add(source_file)

				file_changed = bool(source_files & changed_files)
			except Exception:
				file_changed = False

		if not file_changed:
			continue

		for comment in file_comments[file]:
			try:
				if file_path not in changed_lines_cache:
					changed_lines_cache[file_path] = get_changed_lines(file_path, base_branch, head_branch)

				if comment["line"] in changed_lines_cache[file_path]:
					filtered_comments.append(comment)
			except Exception:
				filtered_comments.append(comment)

	return filtered_comments

def main():
	if "--help" in sys.argv or "-h" in sys.argv:
		print("Usage: style.py [OPTIONS] [FILES...]")
		print("")
		print("Options:")
		print("  -f,                  Automatically fix style issues where possible")
		print("  -ci                  Run in CI mode (outputs JSON for GitHub Actions)")
		print("  -annotations         Emit GitHub Actions inline annotations (::error)")
		print("  --base-branch BRANCH Base branch for comparison (default: master)")
		print("  --head-branch BRANCH Head branch for comparison (default: HEAD)")
		print("  --help, -h           Show this help message")
		print("")
		print("Usage examples:")
		print("  # Check specific files:")
		print("  python3 style.py src/file1.cpp src/file2.h")
		print("")
		print("  # Compare branches:")
		print("  python3 style.py --base-branch master --head-branch feature-branch")
		print("")
		print("  # Auto-fix issues:")
		print("  python3 style.py -f src/file.cpp")
		print("")
		print("  # CI mode:")
		print("  python3 style.py -ci --base-branch master --head-branch feature-branch")
		sys.exit(0)

	fix = False
	ci = False
	annotations = False
	base_branch = None
	head_branch = None
	args = []

	argv = sys.argv[1:]
	i = 0
	while i < len(argv):
		arg = argv[i]
		if arg == "-f":
			fix = True
		elif arg == "-ci":
			ci = True
		elif arg == "-annotations":
			annotations = True
		elif arg == "--base-branch":
			if i + 1 >= len(argv):
				print("Error: --base-branch requires an argument")
				sys.exit(1)
			base_branch = argv[i + 1]
			i += 1
		elif arg == "--head-branch":
			if i + 1 >= len(argv):
				print("Error: --head-branch requires an argument")
				sys.exit(1)
			head_branch = argv[i + 1]
			i += 1
		else:
			args.append(arg)
		i += 1

	# If branch comparison is requested (or nothing else is), fill in defaults.
	if not args:
		if base_branch is None and head_branch is None:
			base_branch = "master"
			head_branch = "HEAD"
		else:
			base_branch = base_branch or "master"
			head_branch = head_branch or "HEAD"

	if base_branch and head_branch and not args:
		changed_files = get_changed_files(base_branch, head_branch)
		cpp_files, h_files, other_files = categorize_files(changed_files)
	else:
		cpp_files, h_files, other_files = categorize_files(args)

	all_files = cpp_files | h_files | other_files

	ciout=None
	if ci:
		try:
			ciout=open(os.environ['GITHUB_OUTPUT'], 'a')
			ciout.write("comments<<EOF\n")
		except KeyError:
			ciout=None

	all_errors = []
	file_comments = {}

	for file in all_files:
		try:
			path = Path(file)
			file_errors = []

			if file in cpp_files | h_files:
				file_errors.extend(check_cpp_file(path, fix))
			else:
				file_errors.extend(check_file(path, fix))

			file_comments[file] = create_file_comments(file_errors, path)

		except Exception as e:
			print(f"Error processing file {file}: {e}")
			if ciout:
				ciout.write(f"Error processing file {file}: {e}\n")
			sys.exit(1)

	try:
		mame_lst_errors = check_mame_lst(cpp_files)
		mame_lst_comments = create_file_comments(mame_lst_errors, Path("src/mame/mame.lst"))
		file_comments["src/mame/mame.lst"] = mame_lst_comments
	except Exception as e:
		print(f"Error processing mame.lst: {e}")
		if ciout:
			ciout.write(f"Error processing mame.lst: {e}\n")
		sys.exit(1)

	try:
		game_entries_errors = check_game_entries_vs_lst(cpp_files)
		game_entries_comments = create_file_comments(game_entries_errors, Path("src/mame/mame.lst"))
		if game_entries_comments:
			if "src/mame/mame.lst" in file_comments:
				file_comments["src/mame/mame.lst"].extend(game_entries_comments)
			else:
				file_comments["src/mame/mame.lst"] = game_entries_comments
	except Exception as e:
		print(f"Error checking game entries vs lst: {e}")
		if ciout:
			ciout.write(f"Error checking game entries vs lst: {e}\n")
		sys.exit(1)

	# Restrict to lines the PR actually touches whenever we have a branch range.
	if (ci or annotations) and base_branch and head_branch:
		comments = filter_comments_for_ci(file_comments, base_branch, head_branch)
	else:
		comments = []
		for file in file_comments:
			comments.extend(file_comments[file])

	if ciout:
		ciout.write(json.dumps(comments) + '\n')
		ciout.write("EOF\n")
		ciout.close()
	elif annotations:
		print_annotations(comments)
	else:
		if comments:
			for comment in comments:
				print(f"File: {comment['path']}")
				print(f"Line: {comment['line']}")
				print(f"Error: {comment['body']}")
				print("-" * 30)
		else:
			print("No style check errors found.")

	# In annotations mode, fail the run when there are issues so the check gates.
	if annotations and comments:
		sys.exit(1)

	sys.exit(0)

if __name__ == "__main__":
	main()
