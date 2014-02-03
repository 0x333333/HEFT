/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package heft;

/**
 *
 * @author zhipeng
 */
public class TaskProcessor {

    int processor;
    double AST;
    double AFT;
    
    public TaskProcessor() {
        
    }

    public TaskProcessor(int processor, double AST, double AFT) {
        this.processor = processor;
        this.AST = AST;
        this.AFT = AFT;
    }

}
