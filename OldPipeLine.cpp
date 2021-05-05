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
bool jump = false;
bool regWrite = false;
bool regDst = false;
bool branch = false;
bool ALUSrc = false;
bool instType = false;
bool memWrite = false;
bool memToReg = false;
bool memRead = false;
int total_clock_cycles = 1;

string registers[32] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
int registerfile[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int d_mem[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
class RegisterFile
{
public:
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
};
RegisterFile RF;

class Execute
{
public:
    void i_jump(bitset<16> instruction)
    {
        string temp = instruction.to_string();
        temp = temp + "00";
        bitset<26> jump_to_address(temp);
        branch_target = bitset<32>((int)(next_pc.to_ullong()) + (int)(binary2long(jump_to_address)));
        zero_output = bitset<1>(1);
    }

    bitset<32> alu(bitset<4> alu_op, bitset<32> input1, bitset<32> input2)
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
};
Execute execute;

class Memory
{
public:
    void write_mem(bitset<32> data_address, bitset<32> write_data)
    {
        d_mem[((int)(data_address.to_ullong())) / 4] = (int)(write_data.to_ullong());
        cout << "memory 0x" << hex << data_address.to_ullong() << " is modified to 0x" << hex << write_data.to_ullong() << endl;
    }
    bitset<32> read_mem(bitset<32> data_address)
    {
        return bitset<32>(d_mem[((int)(data_address.to_ullong())) / 4]);
    }
};
Memory memory;

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
        {                                                 //r
            decode_r();                                   //retrives RS, RT, RD
            reg_values = RF.read_reg(RSadress, RTadress); // retrives register value
            functionType();                               //gets the type of function we will use
            return "r";
        }

        else if (opcode == "000010")
        { //j
            return jumpAdress(instruction);
        }

        else if (opcode == "101011" || opcode == "100011" || opcode == "000100")
        {                                                 // LW(0011) SW(1011)  BEQ(0100)
            I_operand_address();                          //gets RT, RS and IMM's address
            reg_values = RF.read_reg(RSadress, RTadress); //index 0 = RS, index 1 = RT
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

class InstructionMem
{
public:
    vector<bitset<32>> memory_instructions;

    void getInstructions(string filename)
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

class IFStage : public InstructionMem
{
public:
    bool nop;
    bool ocupide = false;
};

class IDStage : Decoder
{
public:
    bool nop;
    bitset<32> instruction;
    bool ocupide = false;
};

class EXEStage
{
public:
    bitset<5> RS;
    bitset<5> RT;
    bitset<5> RD;
    bitset<32> ImmAddress, RSVal, RTVal;
    bitset<16> OGImm;
    bitset<4> alu_op;
    string opcode;
    bool is_I_format;
    bool is_r_format;
    bool nop;
    bool ocupide = false;
};

class MemStage
{
public:
    bitset<32> alu_Result;
    bitset<32> data_address, write_data;
    bitset<5> RS, RT, write_reg;
    bool write_enable;
    bool is_r_format;
    bool is_I_format;
    bool nop;
    bool ocupide = false;
    string opcode;
};

class WBStage
{
public:
    bitset<5> write_reg, RS, RT;
    bitset<32> write_data, alu_result;
    bool nop;
    bool is_execute;
};

class Stages
{
public:
    IFStage IF;
    IDStage ID;
    EXEStage EXE;
    MemStage MEM;
    WBStage WB;
    Stages()
    {
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
bitset<4> controlUnit(string temp_function)
{
    if (temp_function == "100000")
    {
        return add_op;
    }
    else if (temp_function == "100100")
    {
        return and_op;
    }
    else if (temp_function == "100111")
    {
        return nor_op;
    }
    else if (temp_function == "100101")
    {
        return or_op;
    }
    else if (temp_function == "101010")
    {
        return slt_op;
    }
    else if (temp_function == "100010")
    {
        return sub_op;
    }
    return bitset<4>(-111111);
}
int main()
{
    ////Conditions////
    registerfile[9] = 32;
    registerfile[10] = 5;
    registerfile[16] = 112;

    d_mem[28] = 5;
    d_mem[29] = 16;

    string filename = "sample_binary3.txt";
    cout << "Enter the program file name to run:" << endl;
    //cin >> filename;
    cout << "\n";
    bitset<32> instruction;
    Stages cycleState, newCycleState;
    cycleState.IF.nop = false;
    cycleState.EXE.nop = true;
    cycleState.ID.nop = true;
    cycleState.MEM.nop = true;
    cycleState.WB.nop = true;
    cycleState.IF.getInstructions(filename);
    newCycleState = cycleState;
    string format;

    while (true)
    {
        if (cycleState.IF.nop && cycleState.ID.nop && cycleState.EXE.nop && cycleState.MEM.nop && cycleState.WB.nop)
            break;
        cout << "total_clock_cycles " << dec << total_clock_cycles << ":" << endl;

        if (!cycleState.WB.nop)
        {
            if (cycleState.WB.is_execute)
            {
                RF.write_back(cycleState.WB.write_reg, cycleState.WB.write_data);
            }
        }

        newCycleState.WB.nop = cycleState.MEM.nop;
        if (!cycleState.MEM.nop)
        {
            if (cycleState.MEM.is_r_format)
            {
                newCycleState.WB.write_reg = cycleState.MEM.write_reg;
                newCycleState.WB.write_data = cycleState.MEM.alu_Result;
                newCycleState.WB.is_execute = true;
            }
            if (cycleState.MEM.is_I_format && !cycleState.MEM.is_r_format)
            {
                if (cycleState.MEM.opcode == "sw")
                    memory.write_mem(cycleState.MEM.data_address, cycleState.MEM.write_data);

                else if (cycleState.MEM.opcode == "lw")
                {
                    newCycleState.WB.write_reg = cycleState.MEM.write_reg;
                    newCycleState.WB.write_data = memory.read_mem(cycleState.MEM.data_address);
                    newCycleState.WB.is_execute = true;
                }
            }
        }

        newCycleState.MEM.nop = cycleState.EXE.nop;
        if (!cycleState.EXE.nop)
        {
            if (cycleState.EXE.is_r_format)
            {
                newCycleState.MEM.is_r_format = true;
                newCycleState.MEM.write_reg = cycleState.EXE.RD;
                newCycleState.MEM.alu_Result = execute.alu(cycleState.EXE.alu_op, cycleState.EXE.RSVal, cycleState.EXE.RTVal);
            }
            else if (cycleState.EXE.is_I_format)
            {
                newCycleState.MEM.data_address = execute.alu(cycleState.EXE.alu_op, cycleState.EXE.RSVal, cycleState.EXE.ImmAddress);
                newCycleState.MEM.write_reg = cycleState.EXE.RT;
                newCycleState.MEM.opcode = cycleState.EXE.opcode;
                newCycleState.MEM.is_I_format = true;
                //Add the portion of beq
                if (cycleState.EXE.opcode == "beq")
                {
                    if (cycleState.EXE.RSVal.to_ullong() == cycleState.EXE.RSVal.to_ullong())
                        execute.i_jump(cycleState.EXE.OGImm);
                }
            }
        }
        newCycleState.EXE.nop = cycleState.ID.nop;
        if (!cycleState.ID.nop)
        {
            string formatType = decoder.decode(cycleState.ID.instruction);
            if (formatType == "j")
            {
                pc = jump_target;
                next_pc = bitset<32>(pc.to_ullong() + 4);
            }

            else if (formatType == "r")
            {
                newCycleState.EXE.is_r_format = true;
                newCycleState.EXE.alu_op = controlUnit(cycleState.ID.instruction.to_string().substr(26, 6));
                newCycleState.EXE.RS = decoder.RSadress;
                newCycleState.EXE.RT = decoder.RTadress;
                newCycleState.EXE.RD = decoder.RDadress;
                newCycleState.EXE.RSVal = decoder.reg_values[0];
                newCycleState.EXE.RTVal = decoder.reg_values[1];
            }

            else if (formatType.length() > 1)
            {
                string opcode = cycleState.ID.instruction.to_string().substr(0, 6);

                if (opcode == "101011" || opcode == "100011" || opcode == "000100")
                    newCycleState.EXE.alu_op = add_op;

                if (opcode == "100011")
                { //lw
                    newCycleState.EXE.opcode = "lw";
                }
                else if (opcode == "101011")
                { //sw
                    newCycleState.EXE.opcode = "sw";
                }
                else if (opcode == "000100")
                { //BEQ
                    newCycleState.EXE.opcode = "beq";
                }
                newCycleState.EXE.is_I_format = true;
                newCycleState.EXE.RS = decoder.RSadress;
                newCycleState.EXE.RT = decoder.RTadress;
                newCycleState.EXE.RSVal = decoder.reg_values[0];
                newCycleState.EXE.RTVal = decoder.reg_values[1];
                newCycleState.EXE.ImmAddress = decoder.ImmAddress;
                newCycleState.EXE.OGImm = bitset<16>(cycleState.ID.instruction.to_string().substr(16, 32));
            }
        }
        newCycleState.ID.nop = cycleState.IF.nop;

        if (!cycleState.IF.nop)
        {
            newCycleState.ID.instruction = cycleState.IF.fetch_Instruction(pc);

            if (newCycleState.ID.instruction.to_string() == "11111111111111111111111111111111")
            {
                newCycleState.ID.nop = true;
                newCycleState.IF.nop = true;
            }
        }
        increase_pc();
        updatePCValue();
        cycleState = newCycleState;
        total_clock_cycles++;
        cout << "\n";
    }

    cout << "program terminated:" << endl;
    cout << "total execution time is " << dec << total_clock_cycles - 1 << " cycles" << endl;

    return 0;
}