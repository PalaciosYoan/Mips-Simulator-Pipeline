#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include <sstream>
using namespace std;

//initializing global variables that will be used in different locations
bitset<32> pc = bitset<32>(0);
bitset<32> jump_target = bitset<32>(0);
bitset<32> next_pc = bitset<32>(0);
bitset<32> branch_target = bitset<32>(0);
bitset<1> zero_output = bitset<1>(0);
bitset<4> add_op = bitset<4>("0010");
bitset<4> and_op = bitset<4>("0000");
bitset<4> or_op = bitset<4>("0001");
bitset<4> sub_op = bitset<4>("0110");
bitset<4> slt_op = bitset<4>("0111");
bitset<4> nor_op = bitset<4>("1100");
int total_clock_cycles = 1;

string registers[32] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
int registerfile[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 5, 0, 0, 0, 0, 0, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int d_mem[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 16, 0, 0};
//t d_mem[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
//Did this so the function can be called any where of the program
void increase_pc();

template <size_t B>
long binary2long(const bitset<B> &b)
{ //Handles sign binary
    struct
    {
        long x : B;
    } s;
    return s.x = b.to_ulong();
}

vector<bitset<32>> read_write(bitset<5> rd, bitset<5> rd2, bitset<5> write_reg, bitset<32> write_data, bool write_enable = false)
{
    vector<bitset<32>> read;
    read.push_back(bitset<32>((int)(registerfile[rd.to_ullong()])));
    read.push_back(bitset<32>(registerfile[rd2.to_ullong()]));

    if (write_enable)
    {
        registerfile[write_reg.to_ullong()] = (int)(write_data.to_ullong());
        cout << registers[write_reg.to_ullong()] << " is modified to 0x" << hex << write_data.to_ullong() << endl;
    }

    return read;
}

bitset<32> execute(bitset<4> alu_op, bitset<32> input1, bitset<32> input2)
{
    if (alu_op.to_string() == "0010") // add
        return bitset<32>((int)(input1.to_ullong() + input2.to_ullong()));
    else if (alu_op.to_string() == "0000") // and
        return bitset<32>((int)(input1.to_ullong() & input2.to_ullong()));
    else if (alu_op.to_string() == "0001") // or
        return bitset<32>((int)(input1.to_ullong() | input2.to_ullong()));
    else if (alu_op.to_string() == "0110") //sub
        return bitset<32>((int)(input1.to_ullong() - input2.to_ullong()));
    else if (alu_op.to_string() == "") //slt
        return bitset<32>((int)(input1.to_ullong() < input2.to_ullong()));
    else if (alu_op.to_string() == "1100") //nor
        return bitset<32>((int)(~(input1.to_ullong() | input2.to_ullong())));
    return bitset<32>(0);
}

bitset<32> mem(bitset<32> data_address, bitset<32> write_data, bool read_data = true)
{
    if (read_data)
        return bitset<32>(d_mem[((int)(data_address.to_ullong())) / 4]);

    d_mem[((int)(data_address.to_ullong())) / 4] = (int)(write_data.to_ullong());
    cout << "memory 0x" << hex << data_address.to_ullong() << " is modified to 0x" << hex << write_data.to_ullong() << endl;
    return bitset<32>(0);
}

class Iformatt
{
protected:
    bitset<5> RSadress;
    bitset<5> RTadress;
    bitset<16> ImmAddress;

    vector<bitset<32>> read_data;

    bool jump(bitset<32> instruction)
    {
        next_pc = bitset<32>(pc.to_ullong() + 4);
        string temp = instruction.to_string().substr(16, 32);
        temp = temp + "00";
        bitset<26> jump_to_address(temp);
        branch_target = bitset<32>((int)(next_pc.to_ullong()) + (int)(binary2long(jump_to_address)));
        zero_output = bitset<1>(1);
        return true;
    }

    vector<string> operand_address(bitset<32> instruction)
    {
        vector<string> result;
        string rs = instruction.to_string().substr(6, 5);
        string rt = instruction.to_string().substr(11, 5);
        string immediate = instruction.to_string().substr(16, 16);

        result.push_back(rs);
        result.push_back(rt);
        result.push_back(immediate);

        return result;
    }

    bitset<32> final_imm()
    {
        string temp_result;
        if (ImmAddress.to_string().at(0) == '1')
            temp_result = "1111111111111111" + ImmAddress.to_string();

        else if (ImmAddress.to_string().at(0) == '0')
            temp_result = "0000000000000000" + ImmAddress.to_string();

        return bitset<32>(temp_result);
    }

public:
    bool i_formatt(bitset<32> instruction, string opcode)
    {
        vector<string> addresses = operand_address(instruction);
        RSadress = bitset<5>(addresses[0]);
        RTadress = bitset<5>(addresses[1]);
        ImmAddress = bitset<16>(addresses[2]);

        bitset<32> ImmAddress_final = final_imm(); // Not sure if i need this
        //The only reason we would need to use branch_target is for beq
        read_data = read_write(RSadress, bitset<5>(0), bitset<5>(0), bitset<32>(0)); //i=0 => RS i=1 => RT

        bitset<32> data_address = execute(add_op, read_data[0], ImmAddress_final);
        //5  5  5  32
        if (opcode == "100011")
        { //lw
            bitset<32> input1 = mem(data_address, bitset<32>(0));
            read_write(bitset<5>(0), bitset<5>(0), RTadress, input1, true); //updates reg
        }
        else if (opcode == "101011")
        { //sw
            vector<bitset<32>> read_reg = read_write(RTadress, bitset<5>(0), bitset<5>(0), bitset<32>(0));
            mem(data_address, read_reg[0], false); //updates mem
        }
        else if (opcode == "000100")
        { //BEQ
            vector<bitset<32>> read_reg = read_write(bitset<5>(0), RTadress, bitset<5>(0), bitset<32>(0));
            if (read_data[0].to_ullong() == read_reg[1].to_ullong())
            {
                jump(instruction);
            }
        }

        return false;
    }
};
Iformatt i_format;

class rFormat
{
protected:
    vector<bitset<5>> decode_r(bitset<32> instruction)
    {
        vector<bitset<5>> op;
        op.push_back(bitset<5>(instruction.to_string().substr(6, 5)));  //rs
        op.push_back(bitset<5>(instruction.to_string().substr(11, 5))); // rt
        op.push_back(bitset<5>(instruction.to_string().substr(16, 5))); //rd
        return op;
    }

    string functionType(bitset<32> instruction)
    {
        string function = instruction.to_string().substr(26, 6);
        if (function == "100000")
        {
            return "add";
        }
        else if (function == "100100")
        {
            return "and";
        }
        else if (function == "100111")
        {
            return "nor";
        }
        else if (function == "100101")
        {
            return "or";
        }
        else if (function == "101010")
        {
            return "slt";
        }
        else if (function == "100010")
        {
            return "sub";
        }
        return "";
    }

public:
    void r_format(bitset<32> instruction)
    {
        vector<bitset<5>> op = decode_r(instruction);
        string function = functionType(instruction);

        vector<bitset<32>> reg = read_write(op[0], op[1], bitset<5>(0), bitset<32>(0)); //returns rs, rt values
        bitset<32> result;
        if (function == "add")
            result = execute(add_op, reg[0], reg[1]);
        else if (function == "and")
            result = execute(and_op, reg[0], reg[1]);
        else if (function == "nor")
            result = execute(nor_op, reg[0], reg[1]);
        else if (function == "or")
            result = execute(or_op, reg[0], reg[1]);
        else if (function == "slt")
            execute(slt_op, reg[0], reg[1]);
        else if (function == "sub")
            result = execute(sub_op, reg[0], reg[1]);
        read_write(bitset<5>(0), bitset<5>(0), op[2], result, true);
    }
};
rFormat r;

class Decoder : public Iformatt
{
protected:
    string jumpAdress(bitset<32> instruction)
    {
        bitset<28> temp_target = bitset<28>(instruction.to_string().substr(6, 32) + "00");
        jump_target = bitset<32>(next_pc.to_string().substr(0, 4) + temp_target.to_string());
        pc = jump_target;
        next_pc = bitset<32>(pc.to_ullong() + 4);

        cout << "pc is modified to 0x" << hex << pc.to_ullong() << endl;

        return "j";
    }

public:
    string decode(bitset<32> instruction)
    {
        string opcode = instruction.to_string().substr(0, 6);
        if (opcode == "000000")
        { //r
            return "r";
        }

        else if (opcode == "000010")
        { //j
            return jumpAdress(instruction);
        }

        else if (opcode == "101011" || opcode == "100011" || opcode == "000100")
        { // LW(0011) SW(1011)  BEQ(0100)
            return "i" + opcode;
        }

        else
        {
            cout << "Invalid instruction or opcode is not part of the task" << endl;
            exit(1);
        }
        return "";
    }
};
Decoder decoder;

class InstructionMem : public Decoder
{
public:
    vector<bitset<32>> memory_instructions;
    void increase_pc()
    {
        pc = bitset<32>(pc.to_ullong() + 4);
    }

    void updatePCValue()
    {
        if (zero_output.to_string() == "0")
            increase_pc();
        else
        {
            pc = branch_target;
            zero_output = bitset<1>(0);
        }

        cout << "pc is modified to 0x" << hex << pc.to_ullong() << endl;
    }

    InstructionMem()
    {
        ifstream pFile;
        pFile.open("sample_binary.txt");
        string cur_line;
        //memory_instructions.resize(MemSize);

        while (getline(pFile, cur_line))
        {
            memory_instructions.push_back(bitset<32>(cur_line));
        }

        pFile.close();
    }

    bitset<32> fetch_Instruction(bitset<32> address)
    {
        int i = ((int)(address.to_ullong())) / 4; //gets next instruction
        next_pc = bitset<32>(pc.to_ullong() + 4);

        if (i >= memory_instructions.size())
        {
            string temp = "11111111111111111111111111111111";
            bitset<32> a(temp);
            return a;
        }
        return memory_instructions[i];
    }
};

/////////////////////////////////////////////////////////////////////////////
InstructionMem instructions;

int main()
{
    bitset<32> instruction;
    next_pc = bitset<32>(pc.to_ullong() + 4);

    string format;
    bool needsUpdate = true;

    while (true)
    {
        instruction = instructions.fetch_Instruction(pc);
        if (instruction.to_string() == "11111111111111111111111111111111")
            break;
        cout << "total_clock_cycles " << dec << total_clock_cycles << ":" << endl;
        format = decoder.decode(instruction);
        needsUpdate = true;
        if (format == "j")
        {
            needsUpdate = false;
        }

        else if (format == "r")
        {
            r.r_format(instruction);
        }

        else if (format.length() > 1)
        {
            i_format.i_formatt(instruction, format.substr(1, 6));
        }
        if (needsUpdate)
            instructions.updatePCValue();
        cout << "" << endl;
        total_clock_cycles += 1;
    }
    cout << "program terminated:" << endl;
    cout << "total execution time is " << dec << total_clock_cycles - 1 << " cycles" << endl;

    return 0;
}