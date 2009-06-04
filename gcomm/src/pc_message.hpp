#ifndef PC_MESSAGE_HPP
#define PC_MESSAGE_HPP



#include "gcomm/view.hpp"
#include "gcomm/types.hpp"
#include "gcomm/common.hpp"
#include "gcomm/readbuf.hpp"
#include "gcomm/uuid.hpp"

#include "inst_map.hpp"

BEGIN_GCOMM_NAMESPACE

class PCInst
{
    uint32_t last_seq;
    ViewId last_prim;
    uint64_t to_seq; // Reserved for future use
public:

    PCInst() :
        last_seq(-1),
        to_seq(-1)
    {
    }
    
    PCInst(const uint32_t last_seq_,
           const ViewId& last_prim_, 
           const uint64_t to_seq_) :
        last_seq(last_seq_),
        last_prim(last_prim_),
        to_seq(to_seq_)
    {
    }

    uint32_t get_last_seq() const
    {
        return last_seq;
    }

    const ViewId& get_last_prim() const
    {
        return last_prim;
    }

    uint64_t get_to_seq() const
    {
        return to_seq;
    }


    size_t read(const byte_t* buf, const size_t buflen, const size_t offset)
    {
        size_t off = offset;
        if ((off = gcomm::read(buf, buflen, off, &last_seq)) == 0)
            return 0;
        if ((off = last_prim.read(buf, buflen, off)) == 0)
            return 0;
        if ((off = gcomm::read(buf, buflen, off, &to_seq)) == 0)
            return 0;
        return off;
    }
    
    size_t write(byte_t* buf, const size_t buflen, const size_t offset) const
    {
        size_t off = offset;
        if ((off = gcomm::write(last_seq, buf, buflen, off)) == 0)
            return 0;
        if ((off = last_prim.write(buf, buflen, off)) == 0)
            return 0;
        if ((off = gcomm::write(to_seq, buf, buflen, off)) == 0)
            return 0;
        return off;
    }

    static size_t size()
    {
        return 4 + ViewId::size() + 8;
    }
    
};

inline bool operator==(const PCInst& a, const PCInst& b)
{
    return a.get_last_seq() == b.get_last_seq() &&
        a.get_last_prim() == b.get_last_prim() &&
        a.get_to_seq() == b.get_to_seq();
}

typedef InstMap<PCInst> PCInstMap;

class PCMessage
{
public:
    enum Type {T_NONE, T_STATE, T_INSTALL, T_USER};
private:
    int version;
    Type type;
    uint32_t seq;
    
private:
    PCInstMap* inst;
public:
    PCMessage() : 
        version(-1),
        type(T_NONE),
        seq(0),
        inst(0)
    {
    }
    
    PCMessage(const int version_, 
              const Type type_,
              const uint32_t seq_) :
        version(version_),
        type(type_),
        seq(seq_),
        inst(0)
    {
        if (type == T_STATE || type == T_INSTALL)
        {
            inst = new PCInstMap();
        }
    }
    
    PCMessage(const PCMessage& msg)
    {
        *this = msg;
        if (msg.inst != 0)
        {
            inst = new PCInstMap(*msg.inst);
        }
    }
    
    ~PCMessage()
    {
        delete inst;
    }
    
    size_t read(const byte_t* buf, const size_t buflen, const size_t offset)
    {
        size_t off;
        uint32_t b;
        delete inst;
        inst = 0;
        if ((off = gcomm::read(buf, buflen, offset, &b)) == 0)
            return 0;
        version = b & 0xff;
        type = static_cast<Type>((b >> 8) & 0xff);
        if (version != 0)
            return 0;
        if (type <= T_NONE || type > T_USER)
            return 0;
        if ((off = gcomm::read(buf, buflen, off, &seq)) == 0)
            return 0;
        if (type == T_STATE || type == T_INSTALL)
        {
            inst = new PCInstMap();
            if ((off = inst->read(buf, buflen, off)) == 0)
                return 0;
        }
        return off;
    }
    
    size_t write(byte_t* buf, const size_t buflen, const size_t offset) const
    {
        size_t off;
        uint32_t b;
        b = type & 0xff;
        b <<= 8;
        b |= version & 0xff;
        if ((off = gcomm::write(b, buf, buflen, offset)) == 0)
            return 0;
        if ((off = gcomm::write(seq, buf, buflen, off)) == 0)
            return 0;
        if (inst != 0 && (off = inst->write(buf, buflen, off)) == 0)
            return 0;
        return off;        
    }
    
    size_t size() const
    {
        return 4 + 4 + (inst != 0 ? inst->size() : 0);
    }
    
    int get_version() const
    {
        return version;
    }
    
    Type get_type() const
    {
        return type;
    }

    uint16_t get_seq() const
    {
        return seq;
    }
    
    bool has_inst_map() const
    {
        return inst;
    }

    const PCInstMap& get_inst_map() const
    {
        if (has_inst_map() == false)
        {
            throw FatalException("PC message does not have instance map");
        }
        return *inst;
    }

    PCInstMap& get_inst_map()
    {
        if (has_inst_map() == false)
        {
            throw FatalException("PC message does not have instance map");
        }
        return *inst;
    }
};


struct PCStateMessage : PCMessage
{
    PCStateMessage() :
        PCMessage(0, PCMessage::T_STATE, 0)
    {
    }
};

struct PCInstallMessage : PCMessage
{
    PCInstallMessage() :
        PCMessage(0, PCMessage::T_INSTALL, 0)
    {
    }
};

struct PCUserMessage : PCMessage
{
    PCUserMessage(const uint32_t seq) : 
        PCMessage(0, PCMessage::T_USER, 0)
    {
    }
};


inline bool operator==(const PCMessage& a, const PCMessage& b)
{
    bool ret = a.get_version() == b.get_version() &&
        a.get_type() == b.get_type() &&
        a.get_seq() == b.get_seq();
    if (ret == true)
    {
        if (a.has_inst_map() != b.has_inst_map())
        {
            throw FatalException("");
        }
        if (a.has_inst_map())
        {
            ret = ret && a.get_inst_map() == b.get_inst_map();
        }
    }
    return ret;
}

END_GCOMM_NAMESPACE

#endif // PC_MESSAGE_HPP
