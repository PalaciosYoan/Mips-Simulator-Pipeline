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

vector<bitset<32>> read_reg(bitset<5> reg1, bitset<5> reg2)
{
    vector<bitset<32>> read_reg;
    read_reg.push_back(bitset<32>((int)(registerfile[reg1.to_ullong()])));
    read_reg.push_back(bitset<32>((int)(registerfile[reg2.to_ullong()])));
    return read_reg;
}

void write_back(bitset<5> write_reg, bitset<32> write_data)
{
    registerfile[write_reg.to_ullong()] = (int)(write_data.to_ullong());
    cout << registers[write_reg.to_ullong()] << " is modified to 0x" << hex << write_data.to_ullong() << endl;
}
void i_jump(bitset<32> instruction)
{
    string temp = instruction.to_string().substr(16, 32);
    temp = temp + "00";
    bitset<26> jump_to_address(temp);
    branch_target = bitset<32>((int)(next_pc.to_ullong()) + (int)(binary2long(jump_to_address)));
    zero_output = bitset<1>(1);
}

bitset<32> execute(bitset<4> alu_op, bitset<32> input1, bitset<32> input2, bitset<32> instruction, bool branching = false)
{
    if (branching)
    {
        i_jump(instruction);
    }
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

class Decoder
{
    ////////////////////// J type helper functions /////////////////////
    string jumpAdress(bitset<32> instruction)
    {
        bitset<28> temp_target = bitset<28>(instruction.to_string().substr(6, 32) + "00");
        jump_target = bitset<32>(next_pc.to_string().substr(0, 4) + temp_target.to_string());
        return "j";
    }

    ////////////////////// R type helper functions /////////////////////
    void decode_r()
    {

        RSadress = bitset<5>(instruction.to_string().substr(6, 5));
        RTadress = bitset<5>(instruction.to_string().substr(11, 5));
        RDadress = bitset<5>(instruction.to_string().substr(16, 5));
    }

    void functionType()
    {
        string temp_function = instruction.to_string().substr(26, 6);
        if (temp_function == "100000")
        {
            function = "add";
        }
        else if (temp_function == "100100")
        {
            function = "and";
        }
        else if (temp_function == "100111")
        {
            function = "nor";
        }
        else if (temp_function == "100101")
        {
            function = "or";
        }
        else if (temp_function == "101010")
        {
            function = "slt";
        }
        else if (temp_function == "100010")
        {
            function = "sub";
        }
    }
    ////////////////////// I type helper functions /////////////////////
    void I_imm_address(bitset<16> temp_imm)
    { //Handles for negative addresses
        string temp_result;
        if (temp_imm.to_string().at(0) == '1')
            temp_result = "1111111111111111" + temp_imm.to_string();

        else if (temp_imm.to_string().at(0) == '0')
            temp_result = "0000000000000000" + temp_imm.to_string();

        ImmAddress = bitset<32>(temp_result);
    }

    void I_operand_address()
    {
        vector<string> result;
        string rs = instruction.to_string().substr(6, 5);
        string rt = instruction.to_string().substr(11, 5);
        string immediate = instruction.to_string().substr(16, 16);
        RSadress = bitset<5>(rs);
        RTadress = bitset<5>(rt);

        bitset<16> temp_ImmAddress = bitset<16>(immediate);

        I_imm_address(temp_ImmAddress);
    }

public:
    bitset<5> RSadress;
    bitset<5> RTadress;
    bitset<5> RDadress;
    bitset<32> ImmAddress;
    vector<bitset<32>> reg_values; //index 0 = RS, 1 = RT
    string function;

    bitset<32> instruction;

    string decode(bitset<32> instruction)
    {
        this->instruction = instruction;
        string opcode = instruction.to_string().substr(0, 6);
        if (opcode == "000000")
        {                                              //r
            decode_r();                                //retrives RS, RT, RD
            reg_values = read_reg(RSadress, RTadress); // retrives register value
            functionType();                            //gets the type of function we will use
            return "r";
        }

        else if (opcode == "000010")
        { //j
            return jumpAdress(instruction);
        }

        else if (opcode == "101011" || opcode == "100011" || opcode == "000100")
        {                                              // LW(0011) SW(1011)  BEQ(0100)
            I_operand_address();                       //gets RT, RS and IMM's address
            reg_values = read_reg(RSadress, RTadress); //index 0 = RS, index 1 = RT
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

class InstructionMem
{
public:
    vector<bitset<32>> memory_instructions;

    InstructionMem(string filename)
    {
        ifstream pFile;
        pFile.open(filename);
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

void increase_pc()
{
    pc = bitset<32>(pc.to_ullong() + 4);
}

void updatePCValue()
{
    if (zero_output.to_string() == "1")
    {
        pc = branch_target;
        zero_output = bitset<1>(0);
    }

    cout << "pc is modified to 0x" << hex << pc.to_ullong() << endl;
}

/////////////////////////////////////////////////////////////////////////////

int main()
{
    string filename = "sample_binary.txt";
    cout << "Enter the program file name to run:" << endl;
    //cin >> filename;
    cout << "\n";
    bitset<32> instruction;

    InstructionMem fetch_instructions(filename);
    Decoder decoder;

    string format;

    while (true)
    {
        instruction = fetch_instructions.fetch_Instruction(pc);
        increase_pc();

        if (instruction.to_string() == "11111111111111111111111111111111")
            break;

        cout << "total_clock_cycles " << dec << total_clock_cycles << ":" << endl;

        format = decoder.decode(instruction);

        if (format == "j")
        {
            pc = jump_target;
            next_pc = bitset<32>(pc.to_ullong() + 4);
        }

        else if (format == "r")
        {
            bitset<32> result;
            if (decoder.function == "add") //0 = RS 1 = RT
                result = execute(add_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));
            else if (decoder.function == "and")
                result = execute(and_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));
            else if (decoder.function == "nor")
                result = execute(nor_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));
            else if (decoder.function == "or")
                result = execute(or_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));
            else if (decoder.function == "slt")
                execute(slt_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));
            else if (decoder.function == "sub")
                result = execute(sub_op, decoder.reg_values[0], decoder.reg_values[1], bitset<32>(0));

            write_back(decoder.RDadress, result); // 2 = Rd
        }

        else if (format.length() > 1)
        { // I format instruction
            string opcode = format.substr(1, 6);
            bitset<32> data_address = execute(add_op, decoder.reg_values[0], decoder.ImmAddress, bitset<32>(0));

            if (opcode == "100011")
            { //lw
                bitset<32> new_reg_value = mem(data_address, bitset<32>(0));
                write_back(decoder.RTadress, new_reg_value);
            }
            else if (opcode == "101011")
            {                                                    //sw
                mem(data_address, decoder.reg_values[1], false); //updates mem
            }
            else if (opcode == "000100")
            { //BEQ
                if (decoder.reg_values[0].to_ullong() == decoder.reg_values[1].to_ullong())
                    execute(bitset<4>(0), bitset<32>(0), bitset<32>(0), instruction, true);
            }
        }
        updatePCValue(); // This will update the pc value if zero output is 1 and it will always print pc value

        cout << "" << endl;

        total_clock_cycles += 1;
    }

    cout << "program terminated:" << endl;
    cout << "total execution time is " << dec << total_clock_cycles - 1 << " cycles" << endl;

    return 0;
}