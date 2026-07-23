# Verification Walkthrough - Code Editor Enhancements

## Changes Included
1.  **Layout Adjustment**: Fixed the initial splitter ratio to give the Code Editor 75% width and Function List 25% width (previously inverted).
2.  **Rainbow Brackets**: Implementation of rainbow bracket highlighting for `()`, `[]`, and `{}`.
    - Uses distinct colors for 6 levels of nesting.
    - Implemented robust stack-based matching to ensure only correctly paired brackets are highlighted.
    - Handles nested structures (e.g., `if { for { ... } }`).
3.  **Function Name Highlighting**:
    - Implemented specific highlighting for function names (Gold/Orange based on theme).
    - Uses a robust regex-based approach.
    - Added debouncing for performance.

## Verification Steps (Manual)

### 1. Layout Check
- **Action**: Launch the application.
- **Expected Result**: The Function List (left panel) should be significantly narrower than the Code Editor (right panel). The ratio is set to ~1:6 (200px vs 1200px initial size). The function list has a max width of 400px.

### 2. Rainbow Brackets
- **Action**: Open a C++ file with nested logic.
- **Example Code**:
    ```cpp
    void test() {
        if (true) {
            for (int i=0; i<10; i++) {
                // ...
            }
        }
    }
    ```
- **Expected Result**:
    - The outer `{ }` of `test` should be Level 1 color (e.g., Gold).
    - The `( )` of `if` should be Level 1 color.
    - The `{ }` of `if` should be Level 2 color (e.g., Orchid).
    - The `{ }` of `for` should be Level 3 color (e.g., SkyBlue).
    - Deleting a closing brace `}` should update the highlighting (unmatched brackets lose highlight or matching stops).

### 3. Function Highlighting
- **Action**: Observe function definitions in the editor.
- **Expected Result**: Function names like `test`, `setup`, `loop` should be highlighted in a distinct color (Gold/Orange). Keywords like `if`, `while` should NOT be highlighted as function names.

## Technical Details
- **Cleanup**: Removed duplicate legacy implementations of bracket highlighting and fixed function call mismatches (`highlightRainbowBrackets` -> `updateBracketHighlighting`).
- **Performance**: Highlighting is debounced (500ms for functions, ~100ms for brackets) to prevent UI lag during typing.
- **Bug Fixes**:
    - Resolved compilation errors related to `QsciLexerCPP` enums and missing function declarations.
    - Fixed Chinese character garbled text issue by implementing robust UTF-8/Locale encoding detection when opening files.
