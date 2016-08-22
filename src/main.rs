extern crate crypto;

use crypto::digest::Digest;
use crypto::sha1::Sha1;

use std::collections::HashMap;
use std::fs;
use std::io;
use std::io::Read;
use std::path::{Path,PathBuf};

fn get_tree(dir: &PathBuf) -> io::Result<Vec<PathBuf>> {
    println!("    Walking {}", dir.to_str()
             .expect("Invalid filename"));

    let mut files: Vec<PathBuf> = Vec::new();
    if dir.is_dir() {
        for entry in try!(dir.read_dir()) {
            let entry = try!(entry);
            let path = entry.path();
            if path.is_dir() {
                match get_tree(&path) {
                    Ok(mut v) => files.append(&mut v),
                    Err(e) => {return Err(e)},
                }
            } else {
                files.push(path.to_path_buf());
            }
        }
    }

    Ok(files)
}

fn get_hash(file: &Path) -> io::Result<String> {
    println!("    Hashing {}", file.to_str()
             .expect("Invalid filename"));

    let mut f = try!(fs::File::open(file));
    let mut buffer = [0u8; 4096];
    let mut hasher = Sha1::new();
    loop {
        let bytes_read = try!(f.read(&mut buffer[..]));
        if bytes_read > 0 {
            hasher.input(&buffer[0..bytes_read]);
        } else {
            break;
        }
    }

    Ok(hasher.result_str())
}

fn main() {
    let mut hashmap: HashMap<String, Vec<PathBuf>> = HashMap::new();

    println!("Collecting paths");
    let paths = get_tree(&PathBuf::from("."))
        .expect("Could not get file names.");

    println!("Generating hashes");
    for path in paths {
        let hash = get_hash(path.as_path())
            .expect("Error hashing files");

        if hashmap.contains_key(&hash) {
            hashmap.get_mut(&hash).unwrap().push(path.clone());
        } else {
            hashmap.insert(hash, vec![path.clone()]);
        }
    }

    println!("Detecting duplicates");
    for (hash, paths) in &hashmap {
        if paths.len() > 1 {
            println!("Duplicate hash {}", hash);
            for path in paths {
                println!("{}", path.to_str()
                         .expect("Invalid filename."));
            }
            println!("");
        }
    }
}
