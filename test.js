function isObject(x) {
    return typeof x === 'object' && !Array.isArray(x) && x !== null
}
function compare(a, b) {
    // assuming that the keys in a and b are listed in the same order
    const keys_a = Object.keys(a);
    const keys_b = Object.keys(b);

    if (keys_a.length != keys_b.length) return false;
    if (keys_a.length == 0 && keys_b.length == 0) return true;

    for (let i = 0; i < keys_a.length; i++) {
        if (keys_a[i] != keys_b[i]) return false;

        if (isObject(a[keys_a[i]]) && isObject(b[keys_b[i]])) {
            if (!compare(a[keys_a[i]], b[keys_b[i]])) {
                return false;
            }
        } else {
            if (a[keys_a[i]] != b[keys_b[i]]) {
                return false;
            }
        }
    }

    return true;
}

function deepEqual(obj1, obj2) {
  if (obj1 === obj2) return true;

  if (typeof obj1 !== 'object' || obj1 === null || 
      typeof obj2 !== 'object' || obj2 === null) {
    return false;
  }

  const keys1 = Object.keys(obj1);
  const keys2 = Object.keys(obj2);

  if (keys1.length !== keys2.length) return false;

  for (const key of keys1) {
    if (!keys2.includes(key) || !deepEqual(obj1[key], obj2[key])) {
      return false;
    }
  }

  return true;
}


const a = {
    "1": true,
    "2": {
        a: 123,
        b: {
            bool: true
}
}
,
};

const b = {
    "1": true,
    "2": {
        a: 123,
        b: {
            bool: false
}
}
,
};

console.log(deepEqual(a, b));
