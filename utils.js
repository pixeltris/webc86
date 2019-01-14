var domTextOutput = null;

function webcLog(str)
{
    domTextOutput.value += str + "\n";
}

function webcLogRaw(str)
{
    domTextOutput.value += str;
}

function webcLogClear()
{
    domTextOutput.value = "";
}

function memcpy(src, srcOffset, dst, dstOffset, count)
{
    if (useTypedArrays)
    {
        src = src.slice(srcOffset, srcOffset + count);
        dst.set(src, dstOffset);
    }
    else
    {        
        for (var i = 0; i < count; i++)
        {
            dst[dstOffset + i] = src[srcOffset + i];
        }
    }
}

function memset(buffer, bufferOffset, value, count)
{
    if (useTypedArrays)
    {
        buffer.fill(value, bufferOffset, bufferOffset + count);
    }
    else
    {
        for (var i = 0; i < count; i++)
        {
            buffer[bufferOffset + i] = value;
        }
    }
}

function memcmp(buffer1, buffer1Offset, buffer2, buffer2Offset, num)
{
    // Check length?
    for (var i = 0; i < num; i++)
    {
        var v1 = buffer1[buffer1Offset + i];
        var v2 = buffer2[buffer2Offset + i];
        if (v1 != v2)
        {
            return v1 < v2 ? -1 : 1;
        }
    }
    return 0;
}

function memmove(src, srcOffset, dst, dstOffset, count)
{
    if (src == dst && srcOffset == dstOffset)
    {
        return;
    }
    
    if (src != dst || dstOffset < srcOffset)
    {
        memcpy(src, srcOffset, dst, dstOffset, count);
        return;
    }
    
    if (useTypedArrays)
    {
        src.copyWithin(dstOffset, srcOffset, srcOffset + count);
    }
    else
    {
        for (var i = count - 1; i >= 0; i--)
        {
            src[srcOffset + i] = dst[dstOffset + i];
        }
    }
}

// NOTE: PSP doesn't seem to detect the '\0' char, compare using the integer value instead
function getFixedString(buffer, offset, length)
{
    var result = "";
    for (var i = 0; i < length; i++)
    {
        var val = buffer[offset + i];
        if (val == 0)
        {
            break;
        }
        result += String.fromCharCode(val);
    }
    return result;
}

function getCString(buffer, offset)
{
    var result = "";
    for (var i = offset; i < buffer.length; i++)
    {
        var val = buffer[i];
        if (val == 0)
        {
            break;
        }
        result += String.fromCharCode(val);
    }
    return result;
}

function getCStringNumber(buffer, offset, radix)
{
    var result = "";
    for (var i = offset; i < buffer.length; i++)
    {
        var c = String.fromCharCode(buffer[i]);
        
        if((c >= '0' && c <= '9') || c == '-' || c == '+' ||
            (radix == 16 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) ||
            (radix == -1 && c == '.'))// Use -1 for floating point numbers
        {
            result += c;
        }
        else
        {
            break;
        }
    }
    return result;
}

function getInt32(buffer, offset)
{
    return (buffer[offset + 3] << 24) + 
        (buffer[offset + 2] << 16) + 
        (buffer[offset + 1] << 8) + 
        buffer[offset];
}

function setInt32(buffer, offset, val)
{
    buffer[offset] = (val & 0xFF);
    buffer[offset + 1] = (val >> 8) & 0xFF;
    buffer[offset + 2] = (val >> 16) & 0xFF;
    buffer[offset + 3] = (val >> 24) & 0xFF;
}

function getInt16(buffer, offset)
{
    return (buffer[offset + 1] << 8) + buffer[offset];
}

//https://gist.github.com/kg/2192799
function getFloatEx(bytes, signBits, exponentBits, fractionBits, eMin, eMax, littleEndian)
{
    var totalBits = (signBits + exponentBits + fractionBits);

    var binary = "";
    for (var i = 0, l = bytes.length; i < l; i++)
    {
        var bits = bytes[i].toString(2);
        while (bits.length < 8) 
        {
            bits = "0" + bits;
        }

        if (littleEndian)
        {
            binary = bits + binary;
        }
        else
        {
            binary += bits;
        }
    }

    var sign = (binary.charAt(0) == '1')?-1:1;
    var exponent = parseInt(binary.substr(signBits, exponentBits), 2) - eMax;
    var significandBase = binary.substr(signBits + exponentBits, fractionBits);
    var significandBin = '1'+significandBase;
    var i = 0;
    var val = 1;
    var significand = 0;

    if (exponent == -eMax)
    {
        if (significandBase.indexOf('1') == -1)
        {
            return 0;
        }
        else
        {
            exponent = eMin;
            significandBin = '0'+significandBase;
        }
    }
    
    while (i < significandBin.length)
    {
        significand += val * parseInt(significandBin.charAt(i));
        val = val / 2;
        i++;
    }
    
    return sign * significand * Math.pow(2, exponent);
}

function getFloat(bytes, bigEndian)
{
    return getFloatEx(bytes, 1, 8, 23, -126, 127, !bigEndian);
}

function getDouble(bytes, bigEndian)
{
    return getFloatEx(bytes, 1, 11, 52, -1022, 1023, !bigEndian);
}

//https://github.com/kawanet/int64-buffer/blob/master/int64-buffer.js
var BIT32 = 4294967296;
var BIT24 = 16777216;

function uint64ToString(buffer, radix)
{
    return int64ToString(buffer, radix, true);
}

function int64ToString(buffer, radix, unsigned)
{
    var high = getInt32(buffer, 4) >>> 0;
    var low = getInt32(buffer, 0) >>> 0;
    var str = "";
    var sign = !unsigned && (high & 0x80000000);
    if (sign)
    {
        high = ~high;
        low = BIT32 - low;
    }
    radix = radix || 10;
    while (1)
    {
        var mod = (high % radix) * BIT32 + low;
        high = Math.floor(high / radix);
        low = Math.floor(mod / radix);
        str = (mod % radix).toString(radix) + str;
        if (!high && !low) break;
    }
    if (sign)
    {
        str = "-" + str;
    }
    return str;
}

function uint64FromString(str, radix)
{
    return int64FromString(str, radix);
}

function int64FromString(str, radix)
{
    var pos = 0;
    var len = str.length;
    var high = 0;
    var low = 0;
    if (str[0] === "-") pos++;
    var sign = pos;
    radix = radix || 10;
    while (pos < len)
    {
        var chr = parseInt(str[pos++], radix);
        if (!(chr >= 0)) break; // NaN
        low = low * radix + chr;
        high = high * radix + Math.floor(low / BIT32);
        low %= BIT32;
    }
    if (sign)
    {
        high = ~high;
        if (low)
        {
            low = BIT32 - low;
        }
        else
        {
            high++;
        }
    }    
    var result = [0,0,0,0,0,0,0,0];
    setInt32(result, 4, high);
    setInt32(result, 0, low);
    return result;
}

var VA_ARG_I8 = 1;
var VA_ARG_I16 = 2;
var VA_ARG_I32 = 3;
var VA_ARG_I64 = 4;
var VA_ARG_U8 = 5;
var VA_ARG_U16 = 6;
var VA_ARG_U32 = 7;
var VA_ARG_U64 = 8;
var VA_ARG_FLOAT = 9;
var VA_ARG_DOUBLE = 10;
var VA_ARG_STRING = 12;
var VA_ARG_POINTER = 13;

function va_arg(cpu, type)
{
    switch (type)
    {
        case VA_ARG_I8:
            return cpu.popI8();
        case VA_ARG_I16:
            return cpu.popI16();
        case VA_ARG_I32:
            return cpu.popI32();
        case VA_ARG_I64:
            return int64ToString([cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8()]);
        case VA_ARG_U8:
            return cpu.popU8();
        case VA_ARG_U16:
            return cpu.popU16();
        case VA_ARG_U32:
            return cpu.popU32();
        case VA_ARG_U64:
            return uint64ToString([cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8()]);
        case VA_ARG_FLOAT:
            return getFloat([cpu.popI8(),cpu.popI8(),cpu.popI8(),cpu.popI8()]);
        case VA_ARG_DOUBLE:
            return getDouble([cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8(),cpu.popU8()]);
        case VA_ARG_STRING:
            return getCString(cpu.virtualMemory, cpu.popU32() - cpu.virtualMemoryAddress);
        case VA_ARG_POINTER:
            return cpu.popU32();
        default:
            throw "Unhandled va_arg type " + type;
    }
}