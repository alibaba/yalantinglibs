package main

import (
    "fmt"
    "io/ioutil"
    "encoding/binary"
)

type Person struct {
    age uint32
    name string
}

func deserialize_person(src []byte) (*Person) {
    var person Person
    hash_code := binary.LittleEndian.Uint32(src[0:4])
    if hash_code != 2242444774 {
        return nil
    }

    person.age = binary.LittleEndian.Uint32(src[4:]) // when computer is big-end there will be error
    name_size := src[8]
    person.name = string(src[9: 9 + name_size])
    return &person
}

// In general, computer is little-endian
func serialize_person(person Person) (out []byte) {
    var serialize_bytes []byte
    var hash_code uint32
    hash_code = 2242444774
    hash_bytes := make([]byte, 4)       // hashcode 4 bytes
    age_u32_bytes := make([]byte, 4)    // uint32 4 bytes
    var str_size byte                   // str size 1 byte
    // hash code to bytes
    binary.LittleEndian.PutUint32(hash_bytes, hash_code)
    serialize_bytes = append(serialize_bytes, hash_bytes[:]...)
    // field age to bytes
    binary.LittleEndian.PutUint32(age_u32_bytes, person.age)
    serialize_bytes = append(serialize_bytes, age_u32_bytes[:]...)
    // string size to bytes
    str_size = (byte)(len(person.name))
    serialize_bytes = append(serialize_bytes, str_size)
    // field name to bytes
    serialize_bytes = append(serialize_bytes, ([]byte)(person.name)[:]...)

    return serialize_bytes
}


func main() {
    fileName := "../../../../build/examples/example.txt"
    data, err := ioutil.ReadFile(fileName);
    if err != nil {
        fmt.Println("Read file err, err = ", err)
        return
    }
    // 1. deserialize from example.txt
    de_person := deserialize_person(data)
    

    fmt.Println("person age is: ", de_person.age)
    fmt.Println("person name is: ", de_person.name)
    // 2. serialize person to bytes
    serialize_bytes := serialize_person(*de_person)

    fmt.Println(serialize_bytes)
    // 3. deserialize from serialized bytes
    de_person2 := deserialize_person(serialize_bytes)
    fmt.Println("person age is: ", de_person2.age)
    fmt.Println("person name is: ", de_person2.name)

    // 4. write serialized bytes to file
    WritefileName := "../../../../build/examples/golang_serialize.txt"
    err = ioutil.WriteFile(WritefileName, serialize_bytes, 0777)
    if err != nil {
        fmt.Println(err)
    }

}
