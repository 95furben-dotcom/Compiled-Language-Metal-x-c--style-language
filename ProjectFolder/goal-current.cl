static int main(){
    return isPrime(10);
}

static bool isPrime(int x){
    for(int i = 0; i < x / 2; i++){
        if(x % i == 0){
            return false;
        }
    }
    return true;
}