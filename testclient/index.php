<?php

$cSrc = <<<'CODE'
#include <stdio.h>
int main(){
    int a, b;
    scanf("%d%d", &a, &b);
    printf("%d\n", a+b);
    return 0;
}
CODE;
var_dump($cSrc);
// var_dump($_POST['src']);
$ch = curl_init('http://127.0.0.1/index.php');
$defaults = [
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_HEADER => false,
    // set HTTP request header
    CURLOPT_HTTPHEADER => [
        'Content-type: application/json'
    ],
];
curl_setopt_array($this->ch, $defaults);
curl_setopt($ch,CURLOPT_CUSTOMREQUEST,'POST');
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($cSrc));
if (!$result = curl_exec($this->ch)) {
    echo "error";
    trigger_error(curl_error($this->ch));
}
var_dump(json_decode($result, true));