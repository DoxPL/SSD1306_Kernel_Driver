FONT_SET_FILE = 'fontset'
OUTPUT_FILE = 'font.out'
CHAR_WIDTH = 5
SPACES = 4

def read_file():
    lines = []
    with open(FONT_SET_FILE, 'r') as reader:
        for line in reader:
            lines.append(line)
    return lines

def line_to_num_array(line, num):
    num_array = []
    length = len(line)
    index = 0
    for _ in range(CHAR_WIDTH):
        if index >= length or line[index] != '*':
            num_array.append(0)
        else:
            num_array.append(num)
        index += 1
    return num_array

def get_array_line_data(hex_data, curr_chr):
    line_to_write = ''.join(' ' for _ in range(SPACES))
    for val in hex_data:
        line_to_write += (val + ', ')
    return line_to_write + '0x00, // {0}\n'.format(chr(curr_chr))

def get_char_byte_data(bitmap_data):
    ascii_bytes = [0 for _ in range(CHAR_WIDTH)]
    for num_array in bitmap_data:
        index = 0
        for val in num_array:
            ascii_bytes[index] += val
            index += 1
    return ['0x{:02x}'.format(ai_byte).upper() for ai_byte in ascii_bytes]

def convert(content):
    start_chr = 0
    with open(OUTPUT_FILE, 'w') as writer:
        for _ in range(0, 37):
            writer.write('    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // {0}\n'.format(start_chr))
            start_chr = start_chr + 1

        bitmap_data = []
        pow_of_two = 1
        count = 0
        for line in content:
            length = len(line)
            if length <= 1:
                data = get_char_byte_data(bitmap_data)
                writer.write(get_array_line_data(data, start_chr + count))
                bitmap_data.clear()
                pow_of_two = 1
                count += 1
                continue
            bitmap_data.append(line_to_num_array(line, pow_of_two))
            pow_of_two *= 2
        print(count)

if __name__ == '__main__':
    content = read_file()
    convert(content)
