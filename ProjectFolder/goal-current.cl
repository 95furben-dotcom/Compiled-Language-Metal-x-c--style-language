static int main(){
    int a = isPrime(0);
    return a;
}

static bool isPrime(int x){
    for(int i = 2; i < x / 2; i++){
        if(x % i == 0){
            return false;
        }
    }
    return true;
}