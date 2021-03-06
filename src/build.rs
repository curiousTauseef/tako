use std::fs::{self, DirEntry};
use std::io;
use std::io::prelude::{Read, Write};
use std::path::Path;
use std::str::FromStr;

mod cli_options;
mod test_options;
use test_options::TestOptions;
use test_options::TestResult;

// one possible implementation of walking a directory only visiting files
fn visit_dirs(dir: &Path, cb: &mut dyn FnMut(&DirEntry)) -> io::Result<()> {
    if dir.is_dir() {
        for entry in fs::read_dir(dir)? {
            let entry = entry?;
            let path = entry.path();
            if path.is_dir() {
                visit_dirs(&path, cb)?;
            } else {
                cb(&entry);
            }
        }
    }
    Ok(())
}

fn build_test(mut f: &std::fs::File, path: String) {
    let mut test = String::new();
    let mut file = std::fs::File::open(path.to_string()).unwrap();
    file.read_to_string(&mut test).unwrap();

    eprintln!("Building test '{}'", path);
    let opts = TestOptions::from_str(&test).expect("Couldn't read test options");
    let (test_type, expectation) = if opts.expected == TestResult::Panic {
        ("\n#[should_panic]", "".to_owned()) // No result checking needed.
    } else if let TestResult::Output(gold) = opts.expected {
        ("", format!("
    eprintln!(\"Loading golden file {{}}\", \"{gold}\");
    let mut goldfile=std::fs::File::open(\"{gold}\").unwrap();
    let mut golden = String::new();
    goldfile.read_to_string(&mut golden).unwrap();
    eprintln!(\"DONE 3\");
    use pretty_assertions::assert_eq;
    assert_eq!(golden.replace(\"\\r\n\", \"\n\"), format!(\"{{}}{{}}\", stdout.join(\"\"), result));",
    gold = gold))
    } else {
        ("", "".to_owned())
    };

    let fn_name = path.replace("/", "_").replace("\\", "_").replace("._", "");
    write!(
        f,
        "
#[test]{test_type}
fn {fn_name}() {{
    let topts = TestOptions::from_str(\"{opts}\").expect(\"Couldn't read test options\");
    let opts = topts.opts;
    let mut db = DB::default();
    db.set_options(opts);
    for f in db.options().files.iter() {{
        let mut stdout: Vec<String> = vec![];
        let result = {{
            use crate::ast::{{Prim::{{I32, Str}}, Info}};
            use crate::interpreter::Res;
            let mut print_impl = |_: &dyn Compiler, args: Vec<&dyn Fn() -> crate::interpreter::Res>, _: crate::ast::Info| -> Res {{
                stdout.push(match args[0]()? {{
                    Str(s,_)=>s,
                    s=>format!(\"{{:?}}\", s)
                }});
                return Ok(I32(0, Info::default()))
            }};
            super::work(&mut db, &f, Some(&mut print_impl)).expect(\"failed\")
        }};
        eprintln!(\"Result:\\n{{}}\", result);
        {expectation}
        eprintln!(\"DONE 4\");
    }}
}}",
        fn_name = fn_name,
        test_type = test_type,
        opts = test,
        expectation = expectation
    )
    .unwrap();
}

fn files_from(path: &str) -> Vec<String> {
    let mut params: Vec<String> = vec![];
    visit_dirs(Path::new(path), &mut |filename| {
        let pth = filename.path();
        match pth.to_str() {
            Some(s) => {
                if !s.ends_with(".tk")
                    && !s.ends_with(".weird")
                    && !s.ends_with(".sh")
                    && !s.ends_with(".cc")
                    && !s.ends_with(".c")
                {
                    params.push(s.to_string());
                }
            }
            _ => panic!(),
        }
    })
    .expect("Failed finding test files.");
    params
}

fn main() -> std::io::Result<()> {
    let out_dir = std::env::var("OUT_DIR").unwrap();
    let destination = std::path::Path::new(&out_dir).join("test.rs");
    let mut f = std::fs::File::create(&destination).unwrap();

    writeln!(f, "use std::io::prelude::Read;")?;
    writeln!(f, "use std::str::FromStr;")?;
    writeln!(f, "use super::test_options::TestOptions;")?;
    writeln!(f, "use super::database::{{DB, Compiler}};")?;

    for p in files_from("examples") {
        build_test(&f, p);
    }

    for p in files_from("counter_examples") {
        build_test(&f, p);
    }
    Ok(())
}
