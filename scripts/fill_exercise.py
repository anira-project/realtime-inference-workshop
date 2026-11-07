#!/usr/bin/env python3
"""Fills an exercise's ? from its solution, to check that the two still match.

    fill_exercise.py <exercise file> <solution file> <out file>

Every code line of the exercise that holds a ? becomes a pattern: the line as
it is, each ? standing for any code. The next solution line that matches it,
indentation included, takes its place. The result is the exercise as a
participant fills it in, and it has to build and pass like the solution.

Fails, naming the line, when
- a line marked "// <- fill out" (C++) or "// TODO1: ..." (JSON) has no ?:
  the exercise was filled in, or
- no solution line matches a line with ?: exercise and solution drifted apart.

A JSON file's "path" is relative to its own directory: the filled copy lives
elsewhere, so it gets the path made absolute.
"""

import re
import sys
from pathlib import Path

# How far ahead in the solution a given line is looked for: the two files differ
# in comments and in the filled lines only.
k_lookahead = 20


def is_blank(line: str) -> bool:
    """A line the participant fills in: marked in its comment."""
    if "//" not in line or not code_of(line).strip():
        return False
    comment = line.split("//", 1)[1]
    return "fill out" in comment or re.match(r"\s*TODO\d", comment) is not None


def absolute_paths(text: str, directory: Path) -> str:
    """A JSON "path" made absolute, against the directory it was relative to."""
    def absolute(match: re.Match) -> str:
        path = Path(match.group(2))
        if not path.is_absolute():
            path = (directory / path).resolve()
        return f'{match.group(1)}"{path.as_posix()}"'
    return re.sub(r'("path"\s*:\s*)"([^"]+)"', absolute, text)


def code_of(line: str) -> str:
    """The line without its trailing // comment."""
    return line.split("//", 1)[0].rstrip()


def pattern_of(code: str) -> re.Pattern:
    """The code line with every ? standing for any code."""
    return re.compile("".join("(.+)" if part == "?" else re.escape(part)
                              for part in re.split(r"(\?)", code)))


def fill(exercise: Path, solution: Path) -> str:
    exercise_lines = exercise.read_text().splitlines()
    solution_lines = solution.read_text().splitlines()
    errors = []
    filled = []
    next_solution = 0

    for number, line in enumerate(exercise_lines, start=1):
        code = code_of(line)
        if is_blank(line) and "?" not in code:
            errors.append(f"{exercise}:{number}: filled in, expected a ?: {line.strip()}")
        if "?" not in code:
            # Given code is the same in both: walk the solution along with it,
            # so a ? only matches the solution's line at the same place.
            if code.strip():
                for index in range(next_solution, min(next_solution + k_lookahead, len(solution_lines))):
                    if code_of(solution_lines[index]) == code:
                        next_solution = index + 1
                        break
            filled.append(line)
            continue

        pattern = pattern_of(code)
        for index in range(next_solution, len(solution_lines)):
            if pattern.fullmatch(code_of(solution_lines[index])):
                filled.append(code_of(solution_lines[index]))
                next_solution = index + 1
                break
        else:
            errors.append(f"{exercise}:{number}: no line of {solution.name} matches: {line.strip()}")
            filled.append(line)

    if errors:
        sys.exit("\n".join(errors))
    text = "\n".join(filled) + "\n"
    if exercise.suffix == ".json":
        text = absolute_paths(text, exercise.parent)
    return text


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    exercise, solution, out = (Path(argument) for argument in sys.argv[1:])
    text = fill(exercise, solution)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not out.exists() or out.read_text() != text:
        out.write_text(text)


if __name__ == "__main__":
    main()
