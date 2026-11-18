static int main(){ 
    return 0:
}

static bool isPrime(int x){
    for(int i = 2; i < x / 2; i++){
        if(x % i == 0){
            return false;
        }
    }
    return true;
}

static vector<int> PrimeFactorize(int x){
    vector<int> factors;
    while(x % 2 == 0){
        x /= 2;
        factors.push_back(2);
    }
    while(x % 3 == 0){
        x /= 2;
        factors.push_back(2);
    }

    for(int i = 5; i < x / 2; i++){
        if(x % i != 0){
            continue;
        }
        if(isPrime(i)){
            while(x % i != 0){
                factors.push_back(i);
                x /= i;
            }
        }
    }
    return factors;
}