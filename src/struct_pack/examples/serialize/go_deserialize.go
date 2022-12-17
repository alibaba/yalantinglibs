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

func deserialize_person(src []byte) Person {
    var person Person
    person.age = binary.LittleEndian.Uint32(src[4:]) // when computer is big-end there will be error
    name_size := src[8]
    person.name = string(src[9: 9 + name_size])
    return person
}


func main() {
    fileName := "../../../../build/examples/example.txt"
    data, err := ioutil.ReadFile(fileName);
    if err != nil {
        fmt.Println("Read file err, err = ", err)
        return
    }

    de_person := deserialize_person(data)

    fmt.Println("person age is: ", de_person.age)
    fmt.Println("person name is: ", de_person.name)

}