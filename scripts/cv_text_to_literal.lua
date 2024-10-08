-- Converts a text file to a c literal #include-able string
-- Usage: lua file_to_c_string.lua <input_file> <output_file>

-- Check for the correct number of arguments
if #arg < 2 then
    print("Usage: lua file_to_c_string.lua <input_file> <output_file>")
    os.exit(1)
end

-- Get input and output file paths from command-line arguments
local input_file = arg[1]
local output_file = arg[2]

-- Open the input file and read its contents
local file = io.open(input_file, "r")
local content = file:read("*a")
file:close()

-- Escape special characters for C strings
content = content:gsub("\\", "\\\\")
content = content:gsub("\"", "\\\"")
content = content:gsub("\n", "\\n\"\n\"")

-- Write the result as a C string literal to the output file
local output = io.open(output_file, "w")
output:write("\"")
output:write(content)
output:write("\";\n")
output:close()